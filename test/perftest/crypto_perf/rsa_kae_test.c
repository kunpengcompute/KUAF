#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/engine.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/bio.h>

#define PUB_EXPONENT 65537
#define NUM_PROCESSES 128
#define TEST_DURATION 10
#define MAX_OPERATIONS 1000000
#define TEST_DATA "This is a test message for RSA signature performance testing"
#define MAX_KEY_SIZE 8192  // 鏈€澶у瘑閽ユ暟鎹ぇ灏?

// 娴嬭瘯閰嶇疆
typedef struct {
    int key_length;
    const char *name;
} TestConfig;

static TestConfig test_configs[] = {
    {2048, "rsa2048"}, {4096, "rsa4096"}, {0, NULL}  // 缁撴潫鏍囪
};

// 鍏变韩鍐呭瓨鏁版嵁缁撴瀯 - 鍖呭惈棰勭敓鎴愮殑瀵嗛挜鏁版嵁
typedef struct {
    long total_sign_ops;
    long total_verify_ops;
    long total_sign_time;
    long total_verify_time;
    int pid;
    int ready;
    int key_length;

    // 棰勭敓鎴愬瘑閽ユ暟鎹?
    unsigned char rsa_key_data[MAX_KEY_SIZE];
    size_t rsa_key_size;
    int key_loaded;  // 鏍囪瀵嗛挜鏄惁宸插姞杞?
} ProcessStats;

typedef struct {
    unsigned char *signature;
    unsigned char *digest;
    size_t sig_size;
    size_t digest_size;
} SignatureBuffer;

// 绾跨▼灞€閮ㄥ彉閲?
__thread ENGINE *g_engine = NULL;
__thread RSA *g_rsa_key = NULL;
__thread SignatureBuffer g_buf = {0};
__thread int g_key_length = 0;

// 鍏变韩鍐呭瓨鎸囬拡
static ProcessStats *g_stats = NULL;
static const char *SHM_NAME = "/kae_bandwidth_test";

// 楂樼簿搴︽椂闂村嚱鏁?
static uint64_t get_highres_time_us(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000 + ts.tv_nsec / 1000;
}

// 璁＄畻SHA256鎽樿
int compute_sha256_digest(const unsigned char *data, size_t data_len, unsigned char *digest, unsigned int *digest_len)
{
    EVP_MD_CTX *md_ctx = EVP_MD_CTX_new();
    if (!md_ctx)
        return 0;

    if (!EVP_DigestInit_ex(md_ctx, EVP_sha256(), NULL) || !EVP_DigestUpdate(md_ctx, data, data_len) ||
        !EVP_DigestFinal_ex(md_ctx, digest, digest_len)) {
        EVP_MD_CTX_free(md_ctx);
        return 0;
    }
    EVP_MD_CTX_free(md_ctx);
    return 1;
}

// 棰勭敓鎴怰SA瀵嗛挜骞跺簭鍒楀寲鍒板叡浜唴瀛?
int pregenerate_rsa_keys(int key_length)
{
    printf("Pregenerating RSA-%d keys for all processes...\n", key_length);

    BIGNUM *bn = BN_new();
    if (!bn) {
        fprintf(stderr, "Failed to create BIGNUM\n");
        return 0;
    }
    BN_set_word(bn, PUB_EXPONENT);

    RSA *rsa_key = RSA_new();
    if (!rsa_key) {
        fprintf(stderr, "Failed to create RSA key\n");
        BN_free(bn);
        return 0;
    }

    if (!RSA_generate_key_ex(rsa_key, key_length, bn, NULL)) {
        fprintf(stderr, "RSA-%d key generation failed\n", key_length);
        ERR_print_errors_fp(stderr);
        BN_free(bn);
        RSA_free(rsa_key);
        return 0;
    }
    BN_free(bn);

    // 搴忓垪鍖朢SA瀵嗛挜鍒板唴瀛楤IO
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        fprintf(stderr, "Failed to create BIO\n");
        RSA_free(rsa_key);
        return 0;
    }

    // 浣跨敤PEM鏍煎紡搴忓垪鍖栧瘑閽?
    if (!PEM_write_bio_RSAPrivateKey(bio, rsa_key, NULL, NULL, 0, NULL, NULL)) {
        fprintf(stderr, "Failed to serialize RSA key to PEM\n");
        ERR_print_errors_fp(stderr);
        BIO_free(bio);
        RSA_free(rsa_key);
        return 0;
    }

    // 鑾峰彇搴忓垪鍖栧悗鐨勬暟鎹?
    char *bio_data;
    long key_size = BIO_get_mem_data(bio, &bio_data);

    if (key_size <= 0 || key_size > MAX_KEY_SIZE) {
        fprintf(stderr, "Invalid key size: %ld (max: %d)\n", key_size, MAX_KEY_SIZE);
        BIO_free(bio);
        RSA_free(rsa_key);
        return 0;
    }

    printf("RSA-%d key serialized, size: %ld bytes\n", key_length, key_size);

    // 灏嗗簭鍒楀寲鍚庣殑瀵嗛挜鏁版嵁澶嶅埗鍒版墍鏈夎繘绋嬬殑鍏变韩鍐呭瓨妲戒綅
    for (int i = 0; i < NUM_PROCESSES; i++) {
        g_stats[i].rsa_key_size = key_size;
        memcpy(g_stats[i].rsa_key_data, bio_data, key_size);
        g_stats[i].key_length = key_length;
        g_stats[i].key_loaded = 1;  // 鏍囪瀵嗛挜宸插姞杞?
    }

    BIO_free(bio);
    RSA_free(rsa_key);
    printf("RSA-%d keys pregenerated and distributed to shared memory\n", key_length);
    return 1;
}

// 浠庡叡浜唴瀛樺姞杞介鐢熸垚鐨凴SA瀵嗛挜
int load_pregenerated_rsa_key(int process_index)
{
    if (!g_stats[process_index].key_loaded) {
        fprintf(stderr, "No pregenerated key available for process %d\n", process_index);
        return 0;
    }

    // 浠庡唴瀛楤IO鍔犺浇RSA瀵嗛挜
    BIO *bio = BIO_new(BIO_s_mem());
    if (!bio) {
        fprintf(stderr, "Failed to create BIO for key loading\n");
        return 0;
    }

    // 灏嗗叡浜唴瀛樹腑鐨勫瘑閽ユ暟鎹啓鍏IO
    if (BIO_write(bio, g_stats[process_index].rsa_key_data, g_stats[process_index].rsa_key_size) <= 0) {
        fprintf(stderr, "Failed to write key data to BIO\n");
        BIO_free(bio);
        return 0;
    }

    // 浠嶣IO璇诲彇RSA瀵嗛挜
    g_rsa_key = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
    BIO_free(bio);

    if (!g_rsa_key) {
        fprintf(stderr, "Failed to load pregenerated RSA key from PEM data\n");
        ERR_print_errors_fp(stderr);
        return 0;
    }
    return 1;
}

// 鍒濆鍖栨祴璇曠幆澧冿紙浣跨敤棰勭敓鎴愬瘑閽ワ級
int init_test_environment(int pid, int process_index, int key_length)
{
    g_key_length = key_length;

    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
    g_engine = ENGINE_by_id("kae");
    if (!g_engine) {
        fprintf(stderr, "[PID %d] Failed to find KAE engine\n", pid);
        return 0;
    }

    if (!ENGINE_init(g_engine) || !ENGINE_set_default_RSA(g_engine)) {
        fprintf(stderr, "[PID %d] Failed to initialize KAE engine\n", pid);
        return 0;
    }

    // 鍔犺浇棰勭敓鎴愮殑RSA瀵嗛挜锛堜笉鍐嶇幇鍦虹敓鎴愶級
    if (!load_pregenerated_rsa_key(process_index)) {
        fprintf(stderr, "[PID %d] Failed to load RSA-%d key\n", pid, key_length);
        return 0;
    }

    // 棰勫垎閰嶇紦鍐插尯
    int rsa_size = RSA_size(g_rsa_key);
    g_buf.signature = malloc(rsa_size);
    g_buf.digest = malloc(SHA256_DIGEST_LENGTH);
    g_buf.sig_size = rsa_size;
    g_buf.digest_size = SHA256_DIGEST_LENGTH;

    if (!g_buf.signature || !g_buf.digest) {
        fprintf(stderr, "[PID %d] Buffer allocation failed\n", pid);
        return 0;
    }

    // 棰勮绠楁憳瑕?
    int test_data_len = strlen(TEST_DATA);
    unsigned int digest_len;
    if (!compute_sha256_digest((unsigned char *)TEST_DATA, test_data_len, g_buf.digest, &digest_len)) {
        fprintf(stderr, "[PID %d] Digest computation failed\n", pid);
        return 0;
    }
    return 1;
}

// 娓呯悊娴嬭瘯鐜
void cleanup_test_environment(void)
{
    if (g_buf.signature)
        free(g_buf.signature);
    if (g_buf.digest)
        free(g_buf.digest);
    if (g_rsa_key)
        RSA_free(g_rsa_key);
    if (g_engine) {
        ENGINE_finish(g_engine);
        ENGINE_free(g_engine);
    }
}

// 绛惧悕鎬ц兘娴嬭瘯
void run_sign_performance_test(int pid, int process_index, int key_length)
{
    unsigned int sig_len;
    long operations = 0;
    uint64_t start_time, end_time;
    uint64_t test_end_time;

    start_time = get_highres_time_us();
    test_end_time = start_time + TEST_DURATION * 1000000;

    while ((end_time = get_highres_time_us()) < test_end_time && operations < MAX_OPERATIONS) {
        if (RSA_sign(NID_sha256, g_buf.digest, SHA256_DIGEST_LENGTH, g_buf.signature, &sig_len, g_rsa_key)) {
            operations++;
        }
    }

    uint64_t actual_test_time = end_time - start_time;

    // 鐩存帴鍐欏叆鍏变韩鍐呭瓨
    g_stats[process_index].total_sign_ops = operations;
    g_stats[process_index].total_sign_time = actual_test_time;
    g_stats[process_index].pid = pid;
    g_stats[process_index].key_length = key_length;
    __sync_synchronize();  // 鍐呭瓨灞忛殰纭繚鏁版嵁鍚屾
    g_stats[process_index].ready = 1;
}

// 楠岀鎬ц兘娴嬭瘯
void run_verify_performance_test(int pid, int process_index, int key_length)
{
    unsigned int sig_len;
    long operations = 0;
    uint64_t start_time, end_time;
    uint64_t test_end_time;

    // 棣栧厛鐢熸垚涓€涓湁鏁堢殑绛惧悕
    if (!RSA_sign(NID_sha256, g_buf.digest, SHA256_DIGEST_LENGTH, g_buf.signature, &sig_len, g_rsa_key)) {
        fprintf(stderr, "[PID %d] Failed to generate initial signature for verify test\n", pid);
        return;
    }

    start_time = get_highres_time_us();
    test_end_time = start_time + TEST_DURATION * 1000000;

    while ((end_time = get_highres_time_us()) < test_end_time && operations < MAX_OPERATIONS) {
        if (RSA_verify(NID_sha256, g_buf.digest, SHA256_DIGEST_LENGTH, g_buf.signature, sig_len, g_rsa_key)) {
            operations++;
        }
    }

    uint64_t actual_test_time = end_time - start_time;

    // 鐩存帴鍐欏叆鍏变韩鍐呭瓨
    g_stats[process_index].total_verify_ops = operations;
    g_stats[process_index].total_verify_time = actual_test_time;
    __sync_synchronize();  // 鍐呭瓨灞忛殰纭繚鏁版嵁鍚屾
}

// 鏀堕泦骞舵墦鍗扮壒瀹氬瘑閽ラ暱搴︾殑缁撴灉
void print_final_results_for_keylength(int key_length, const char *test_type)
{
    long total_ops = 0;
    double max_time_us = 0;
    int ready_processes = 0;

    // 鏀堕泦鐗瑰畾瀵嗛挜闀垮害鐨勭粺璁′俊鎭?
    for (int i = 0; i < NUM_PROCESSES; i++) {
        if (g_stats[i].ready && g_stats[i].key_length == key_length) {
            long ops = (strcmp(test_type, "sign") == 0) ? g_stats[i].total_sign_ops : g_stats[i].total_verify_ops;
            long time_us = (strcmp(test_type, "sign") == 0) ? g_stats[i].total_sign_time : g_stats[i].total_verify_time;

            total_ops += ops;
            if (time_us > max_time_us) {
                max_time_us = time_us;
            }
            ready_processes++;
        }
    }

    if (ready_processes == 0 || max_time_us == 0) {
        printf("No valid results collected for RSA-%d %s test\n", key_length, test_type);
        return;
    }

    // 璁＄畻鍚炲悙閲?
    double throughput = (double)total_ops * 1000000.0 / max_time_us;

    printf("\n");
    printf("+------------------------------------------------+\n");
    printf("| %-46s |\n", "RSA Performance Results (Shared Keys)");
    printf("+------------------------------------------------+\n");
    printf("| %-20s | %-22s |\n", "Parameter", "Value");
    printf("+------------------------------------------------+\n");
    printf("| %-20s | %-22d |\n", "Key Length", key_length);
    printf("| %-20s | %-22d |\n", "Processes", ready_processes);
    printf("| %-20s | %-22s |\n", "Test Type", test_type);
    printf("| %-20s | %-22ld |\n", "Total Operations", total_ops);
    printf("| %-20s | %-22.2f |\n", "Test Time (s)", max_time_us / 1000000.0);
    printf("| %-20s | %-22.2f |\n", "Throughput (ops/s)", throughput);
    printf("| %-20s | %-22.2f |\n", "Per-process (ops/s)", throughput / ready_processes);
    printf("+------------------------------------------------+\n");
}

// 鎬ц兘瀵规瘮鍒嗘瀽
void print_comparison_analysis(void)
{
    printf("\n");
    printf("==================================================\n");
    printf("PERFORMANCE COMPARISON ANALYSIS (SHARED KEYS)\n");
    printf("==================================================\n");

    // 鏀堕泦鎵€鏈夋祴璇曢厤缃殑缁撴灉
    for (int cfg = 0; test_configs[cfg].key_length != 0; cfg++) {
        int key_length = test_configs[cfg].key_length;
        const char *name = test_configs[cfg].name;

        long sign_ops = 0, verify_ops = 0;
        long sign_time = 0, verify_time = 0;
        int sign_processes = 0, verify_processes = 0;

        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (g_stats[i].ready && g_stats[i].key_length == key_length) {
                if (g_stats[i].total_sign_ops > 0) {
                    sign_ops += g_stats[i].total_sign_ops;
                    sign_time += g_stats[i].total_sign_time;
                    sign_processes++;
                }
                if (g_stats[i].total_verify_ops > 0) {
                    verify_ops += g_stats[i].total_verify_ops;
                    verify_time += g_stats[i].total_verify_time;
                    verify_processes++;
                }
            }
        }

        if (sign_processes > 0 && verify_processes > 0) {
            double sign_throughput = (double)sign_ops * 1000000.0 / sign_time;
            double verify_throughput = (double)verify_ops * 1000000.0 / verify_time;
            double sign_avg_time = (double)sign_time / sign_ops;
            double verify_avg_time = (double)verify_time / verify_ops;

            printf("\n%s:\n", name);
            printf("  Sign:     %8.2f ops/sec, %6.2f us/op\n", sign_throughput, sign_avg_time);
            printf("  Verify:   %8.2f ops/sec, %6.2f us/op\n", verify_throughput, verify_avg_time);
            printf("  Ratio:    Sign/Verify = %.2f:1 (throughput)\n", verify_throughput / sign_throughput);
            printf("  Ratio:    Sign/Verify = %.2f:1 (time)\n", sign_avg_time / verify_avg_time);
        }
    }

    // 璁＄畻RSA4096鐩稿浜嶳SA2048鐨勬€ц兘姣斾緥
    double rsa2048_sign_tp = 0, rsa4096_sign_tp = 0;
    double rsa2048_verify_tp = 0, rsa4096_verify_tp = 0;

    for (int cfg = 0; test_configs[cfg].key_length != 0; cfg++) {
        int key_length = test_configs[cfg].key_length;
        long sign_ops = 0, verify_ops = 0;
        long sign_time = 0, verify_time = 0;

        for (int i = 0; i < NUM_PROCESSES; i++) {
            if (g_stats[i].ready && g_stats[i].key_length == key_length) {
                sign_ops += g_stats[i].total_sign_ops;
                sign_time += g_stats[i].total_sign_time;
                verify_ops += g_stats[i].total_verify_ops;
                verify_time += g_stats[i].total_verify_time;
            }
        }

        if (key_length == 2048) {
            rsa2048_sign_tp = (double)sign_ops * 1000000.0 / sign_time;
            rsa2048_verify_tp = (double)verify_ops * 1000000.0 / verify_time;
        } else if (key_length == 4096) {
            rsa4096_sign_tp = (double)sign_ops * 1000000.0 / sign_time;
            rsa4096_verify_tp = (double)verify_ops * 1000000.0 / verify_time;
        }
    }

    if (rsa2048_sign_tp > 0 && rsa4096_sign_tp > 0) {
        printf("\nRSA4096 vs RSA2048 Performance Ratio:\n");
        printf("  Sign:   %.2f%% of RSA2048 performance\n", (rsa4096_sign_tp / rsa2048_sign_tp) * 100);
        printf("  Verify: %.2f%% of RSA2048 performance\n", (rsa4096_verify_tp / rsa2048_verify_tp) * 100);
    }
}

// 瀛愯繘绋嬩富鍑芥暟
void child_process(int process_index, int key_length)
{
    int pid = getpid();

    unsigned int seed = (unsigned int)(pid ^ time(NULL));
    int random_delay = rand_r(&seed) % 3000000;  // 0-3绉掔殑闅忔満寤惰繜锛堝井绉掞級
    printf("Process %d (PID: %d) waiting %d ms before starting\n", process_index, pid, random_delay / 1000);

    usleep(random_delay);  // 闅忔満寤惰繜

    if (!init_test_environment(pid, process_index, key_length)) {
        fprintf(stderr, "[PID %d] Failed to initialize test environment\n", pid);
        exit(1);
    }

    // 绛夊緟鎵€鏈夎繘绋嬪氨缁紙鍑忓皯绛夊緟鏃堕棿锛屽洜涓哄凡缁忔湁闅忔満寤惰繜锛?
    usleep(500000);  // 0.5绉?

    if (!init_test_environment(pid, process_index, key_length)) {
        fprintf(stderr, "[PID %d] Failed to initialize test environment\n", pid);
        exit(1);
    }

    // 绛夊緟鎵€鏈夎繘绋嬪氨缁?
    sleep(1);

    // 杩愯绛惧悕鎬ц兘娴嬭瘯
    run_sign_performance_test(pid, process_index, key_length);

    // 鐭殏绛夊緟纭繚鏁版嵁鍚屾
    usleep(100000);

    // 杩愯楠岀鎬ц兘娴嬭瘯
    run_verify_performance_test(pid, process_index, key_length);

    cleanup_test_environment();
    exit(0);
}

// 鍒濆鍖栧叡浜唴瀛?
int init_shared_memory(void)
{
    int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open failed");
        return 0;
    }

    if (ftruncate(shm_fd, NUM_PROCESSES * sizeof(ProcessStats)) == -1) {
        perror("ftruncate failed");
        return 0;
    }

    g_stats = mmap(NULL, NUM_PROCESSES * sizeof(ProcessStats), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (g_stats == MAP_FAILED) {
        perror("mmap failed");
        return 0;
    }

    // 鍒濆鍖栧叡浜唴瀛?
    memset(g_stats, 0, NUM_PROCESSES * sizeof(ProcessStats));
    return 1;
}

// 娓呯悊鍏变韩鍐呭瓨
void cleanup_shared_memory(void)
{
    if (g_stats) {
        munmap(g_stats, NUM_PROCESSES * sizeof(ProcessStats));
    }
    shm_unlink(SHM_NAME);
}

// 杩愯鐗瑰畾瀵嗛挜闀垮害鐨勬祴璇?
void run_test_for_keylength(int key_length)
{
    printf("\n");
    printf("==================================================\n");
    printf("STARTING RSA-%d PERFORMANCE TEST (SHARED KEYS)\n", key_length);
    printf("==================================================\n");

    // 閲嶆柊鍒濆鍖栧叡浜唴瀛?
    memset(g_stats, 0, NUM_PROCESSES * sizeof(ProcessStats));

    // 棰勭敓鎴愬瘑閽ワ紙涓昏繘绋嬩竴娆＄敓鎴愶紝鎵€鏈夊瓙杩涚▼鍏变韩锛?
    if (!pregenerate_rsa_keys(key_length)) {
        fprintf(stderr, "Failed to pregenerate RSA-%d keys, aborting test\n", key_length);
        return;
    }

    struct timeval prog_start, prog_end;
    gettimeofday(&prog_start, NULL);

    // 鍒涘缓瀛愯繘绋?
    pid_t pids[NUM_PROCESSES];
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pids[i] = fork();
        if (pids[i] == 0) {
            child_process(i, key_length);
        } else if (pids[i] < 0) {
            perror("fork failed");
            continue;
        }
    }

    // 绛夊緟鎵€鏈夊瓙杩涚▼瀹屾垚
    int completed = 0;
    while (completed < NUM_PROCESSES) {
        int status;
        pid_t pid = wait(&status);
        if (pid > 0) {
            completed++;
        }
    }

    gettimeofday(&prog_end, NULL);
    long total_time = (prog_end.tv_sec - prog_start.tv_sec) * 1000000 + (prog_end.tv_usec - prog_start.tv_usec);

    // 鎵撳嵃璇ュ瘑閽ラ暱搴︾殑缁撴灉
    printf("\n");
    printf("==================================================\n");
    printf("RSA-%d FINAL RESULTS (SHARED KEYS)\n", key_length);
    printf("==================================================\n");

    print_final_results_for_keylength(key_length, "sign");
    print_final_results_for_keylength(key_length, "verify");

    printf("\nRSA-%d total test time: %.2f seconds\n", key_length, total_time / 1000000.0);
}

int main()
{
    printf("=== KAE RSA Multi-Process Bandwidth Test (SHARED KEYS) ===\n");
    printf("Processes: %d, Duration: %d seconds\n", NUM_PROCESSES, TEST_DURATION);
    printf("Test Keys: ");
    for (int i = 0; test_configs[i].key_length != 0; i++) {
        printf("%s ", test_configs[i].name);
    }
    printf("\nTest mode: Shared pregenerated keys for maximum performance\n\n");

    // 鍒濆鍖栧叡浜唴瀛?
    if (!init_shared_memory()) {
        fprintf(stderr, "Failed to initialize shared memory\n");
        exit(1);
    }

    // 杩愯鎵€鏈夋祴璇曢厤缃?
    for (int i = 0; test_configs[i].key_length != 0; i++) {
        run_test_for_keylength(test_configs[i].key_length);
    }

    // 鎵撳嵃鎬ц兘瀵规瘮鍒嗘瀽
    print_comparison_analysis();

    // 娓呯悊鍏变韩鍐呭瓨
    cleanup_shared_memory();

    EVP_cleanup();
    ERR_free_strings();
    return 0;
}