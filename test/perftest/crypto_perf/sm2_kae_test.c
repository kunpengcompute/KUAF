/*
 * @Copyright: Copyright (c) Huawei Technologies Co., Ltd. 2024-2024. All rights reserved.
 * @Description: sm2 functest try with performance test - multi-process version
 * @Author: LiuYongYang
 * @Date: 2024-03-21
 * @LastEditTime: 2024-03-25
 */

#include <cstdio>
#include <iostream>
#include <string>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <openssl/ec.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cstring>

using std::cout;
using std::endl;
using std::string;

// 鎬ц兘娴嬭瘯閰嶇疆
const int TEST_DURATION = 5;           // 娴嬭瘯鏃堕暱(绉?
const int NUM_PROCESSES = 128;           // 杩涚▼鏁?
const int BATCH_SIZE = 100;            // 鎵归噺鎿嶄綔澶у皬
const int TEST_DATA_SIZE = 10240;       // 娴嬭瘯鏁版嵁澶у皬

// 鍏变韩鍐呭瓨缁撴瀯
typedef struct {
    long total_sign_ops;
    long total_verify_ops;
    long total_encrypt_ops;
    long total_decrypt_ops;
    int processes_completed;
    pid_t process_pids[NUM_PROCESSES];
    int ready;
} shared_data_t;

shared_data_t *shared_data = NULL;

static inline void PrintBufferHex(const char* buffer, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (i % 10 == 0) {
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "%#04x\t", buffer[i]);
    }
    fprintf(stderr, "\n");
}

// 鑾峰彇褰撳墠鏃堕棿(寰)
double get_current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec * 1000000.0 + (double)tv.tv_usec;
}

// 鍒濆鍖栧叡浜唴瀛?
int init_shared_memory() {
    int shm_fd = shm_open("/sm2_perf_test", O_CREAT | O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        return -1;
    }
    
    if (ftruncate(shm_fd, sizeof(shared_data_t)) == -1) {
        perror("ftruncate");
        return -1;
    }
    
    shared_data = (shared_data_t *)mmap(NULL, sizeof(shared_data_t), PROT_READ | PROT_WRITE, 
                      MAP_SHARED, shm_fd, 0);
    if (shared_data == MAP_FAILED) {
        perror("mmap");
        return -1;
    }
    
    memset(shared_data, 0, sizeof(shared_data_t));
    shared_data->ready = 0;
    
    return 0;
}

// 娓呯悊鍏变韩鍐呭瓨
void cleanup_shared_memory() {
    if (shared_data) {
        munmap(shared_data, sizeof(shared_data_t));
        shm_unlink("/sm2_perf_test");
    }
}

EVP_PKEY* SM2_CreateEVP_PKEY(const string& keyStr, bool isPublic)
{
    BIO* bio_key = BIO_new_mem_buf(keyStr.c_str(), -1);
    EVP_PKEY* evp_pkey = isPublic ?
        PEM_read_bio_PUBKEY(bio_key, NULL, NULL, NULL) : PEM_read_bio_PrivateKey(bio_key, NULL, NULL, NULL);

    if (evp_pkey) {
        EVP_PKEY_set_alias_type(evp_pkey, EVP_PKEY_SM2);
    }
    BIO_free_all(bio_key);
    return evp_pkey;
}

void SM2_GenKey(string& prikeyStr, string& pubkeyStr)
{
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_sm2);
    EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(NID_sm2);
    EC_KEY_set_group(ec_key, ec_group);

    EC_KEY_generate_key(ec_key);
    BIO* bpri_key = BIO_new(BIO_s_mem());
    BIO* bpub_key = BIO_new(BIO_s_mem());
    PEM_write_bio_ECPrivateKey(bpri_key, ec_key, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_EC_PUBKEY(bpub_key, ec_key);

    size_t prk_len = BIO_pending(bpri_key);
    size_t pbk_len = BIO_pending(bpub_key);
    fprintf(stderr, "private_key_len = %lu, public_key_len = %lu\n", prk_len, pbk_len);

    char *prkBuff = new char[prk_len + 1]();
    char *pbkBuff = new char[pbk_len + 1]();
    BIO_read(bpri_key, prkBuff, prk_len);
    BIO_read(bpub_key, pbkBuff, pbk_len);
    prikeyStr = prkBuff;
    pubkeyStr = pbkBuff;

free_mem:
    delete[] pbkBuff;
    delete[] prkBuff;
    BIO_free_all(bpub_key);
    BIO_free_all(bpri_key);
    EC_GROUP_free(ec_group);
    EC_KEY_free(ec_key);
}

string SM2_enc(EVP_PKEY* pubKey, const string& message, ENGINE* e = NULL)
{
    size_t encSize = 1024;
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(pubKey, e);
    if (!pkey_ctx) {
        cout << "鍒涘缓鍔犲瘑涓婁笅鏂囧け璐? << endl;
        return "";
    }
    
    EVP_PKEY_encrypt_init(pkey_ctx);
    if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_MD, -1, (void *)EVP_sm3()) <= 0) {
        cout << "SM2 EVP_PKEY_CTX_ctr control pctx failed" << endl;
    }

    const unsigned char* pMsg = (const unsigned char*)message.c_str();
    unsigned char* pEncMsg = new unsigned char[1024+1]();
    if (EVP_PKEY_encrypt(pkey_ctx, pEncMsg, &encSize, pMsg, message.size()) <= 0) {
        cout << "SM2鍔犲瘑澶辫触" << endl;
        delete[] pEncMsg;
        EVP_PKEY_CTX_free(pkey_ctx);
        return "";
    }
    
    string retStr((const char*)pEncMsg, encSize);

    delete[] pEncMsg;
    EVP_PKEY_CTX_free(pkey_ctx);
    return retStr;
}

string SM2_dec(EVP_PKEY* priKey, const string& encMsg, ENGINE* e = NULL)
{
    size_t decSize = 1024;
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(priKey, e);
    if (!pkey_ctx) {
        cout << "鍒涘缓瑙ｅ瘑涓婁笅鏂囧け璐? << endl;
        return "";
    }
    
    EVP_PKEY_decrypt_init(pkey_ctx);
    if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_MD, -1, (void *)EVP_sm3()) <= 0) {
        cout << "SM2 EVP_PKEY_CTX_ctr control pctx failed" << endl;
    }

    const unsigned char* pMsg = (const unsigned char*)encMsg.c_str();
    unsigned char* pDecMsg = new unsigned char[1024+1]();
    if (EVP_PKEY_decrypt(pkey_ctx, pDecMsg, &decSize, pMsg, encMsg.size()) <= 0) {
        cout << "SM2瑙ｅ瘑澶辫触" << endl;
        delete[] pDecMsg;
        EVP_PKEY_CTX_free(pkey_ctx);
        return "";
    }
    
    string retStr((const char*)pDecMsg, decSize);

    delete[] pDecMsg;
    EVP_PKEY_CTX_free(pkey_ctx);
    return retStr;
}

string SM2_sign(EVP_PKEY* priKey, const string& message, const string& sm2_id, ENGINE* e = NULL)
{
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        cout << "鍒涘缓绛惧悕涓婁笅鏂囧け璐? << endl;
        return "";
    }
    
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(priKey, e);
    if (!pkey_ctx) {
        cout << "鍒涘缓PKEY涓婁笅鏂囧け璐? << endl;
        EVP_MD_CTX_free(md_ctx);
        return "";
    }
    
    EVP_PKEY_CTX_set1_id(pkey_ctx, (const unsigned char*)sm2_id.c_str(), sm2_id.size());
    EVP_MD_CTX_set_pkey_ctx(md_ctx, pkey_ctx);

    if (EVP_DigestSignInit(md_ctx, NULL, EVP_sm3(), e, priKey) != 1) {
        cout << "绛惧悕鍒濆鍖栧け璐? << endl;
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_CTX_free(pkey_ctx);
        return "";
    }

    size_t sign_len = 1024;
    const unsigned char* pmsg = (const unsigned char*)message.c_str();
    unsigned char* signBuff = new unsigned char[1024 + 1]();

    if (EVP_DigestSign(md_ctx, signBuff, &sign_len, pmsg, message.size()) != 1) {
        cout << "绛惧悕澶辫触" << endl;
        delete[] signBuff;
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_CTX_free(pkey_ctx);
        return "";
    }
    
    string retStr((const char*)signBuff, sign_len);

    delete[] signBuff;
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_CTX_free(pkey_ctx);
    return retStr;
}

void SM2_verify(EVP_PKEY* pubKey, const string& message, const string& signMsg, const string& sm2_id,
    ENGINE* e = NULL)
{
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    if (!md_ctx) {
        cout << "鍒涘缓楠岀涓婁笅鏂囧け璐? << endl;
        return;
    }
    
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(pubKey, e);
    if (!pkey_ctx) {
        cout << "鍒涘缓PKEY涓婁笅鏂囧け璐? << endl;
        EVP_MD_CTX_free(md_ctx);
        return;
    }
    
    EVP_PKEY_CTX_set1_id(pkey_ctx, (const unsigned char*)sm2_id.c_str(), sm2_id.size());
    EVP_MD_CTX_set_pkey_ctx(md_ctx, pkey_ctx);

    if (EVP_DigestVerifyInit(md_ctx, NULL, EVP_sm3(), e, pubKey) != 1) {
        cout << "楠岀鍒濆鍖栧け璐? << endl;
        EVP_MD_CTX_free(md_ctx);
        EVP_PKEY_CTX_free(pkey_ctx);
        return;
    }
    
    const unsigned char* pSign = (const unsigned char*)signMsg.c_str();
    const unsigned char* pmesg = (const unsigned char*)message.c_str();
    // if (EVP_DigestVerify(md_ctx, pSign, signMsg.size(), pmesg, message.size()) != 1) {
    //     cout << "SM2 signature verify failed!" << endl;
    // } else {
    //     cout << "SM2 signature verify succeeded!" << endl;
    // }
int ret = EVP_DigestVerify(md_ctx, pSign, signMsg.size(), pmesg, message.size());
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_CTX_free(pkey_ctx);
}

// 鍒濆鍖朘AE寮曟搸 - 姣忎釜杩涚▼鐙珛鍒濆鍖?
ENGINE* init_kae_engine() {
    
    // 纭繚OpenSSL鍒濆鍖?
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
    
    // 鍔犺浇鍔ㄦ€佸紩鎿?
    ENGINE_load_dynamic();
    
    // 鑾峰彇KAE寮曟搸
    ENGINE* engine = ENGINE_by_id("kae");
    if (!engine) {
        printf("杩涚▼ %d: KAE寮曟搸鏈壘鍒帮紝浣跨敤杞欢瀹炵幇\n", getpid());
        return NULL;
    }
    
    // 鍒濆鍖栧紩鎿?
    if (!ENGINE_init(engine)) {
        printf("杩涚▼ %d: KAE寮曟搸鍒濆鍖栧け璐ワ紝浣跨敤杞欢瀹炵幇\n", getpid());
        ENGINE_free(engine);
        return NULL;
    }
    
    // 璁剧疆KAE涓洪粯璁ゅ紩鎿?
    if (!ENGINE_set_default(engine, ENGINE_METHOD_ALL)) {
        printf("杩涚▼ %d: 璁剧疆KAE涓洪粯璁ゅ紩鎿庡け璐ワ紝浣跨敤杞欢瀹炵幇\n", getpid());
        ENGINE_finish(engine);
        ENGINE_free(engine);
        return NULL;
    }
    return engine;
}

// 娓呯悊寮曟搸
void cleanup_engine(ENGINE* engine) {
    if (engine) {
        ENGINE_finish(engine);
        ENGINE_free(engine);
        // 閲嶆柊鍒濆鍖栧唴缃紩鎿?
        ENGINE_cleanup();
        OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
    }
}

// 绛惧悕鎬ц兘娴嬭瘯
void run_sign_perf_test(EVP_PKEY* priKey, const string& message, const string& sm2_id, 
                       ENGINE* engine, int process_id) {
    
    long sign_count = 0;
    double start_time = get_current_time_us();
    double end_time = start_time + TEST_DURATION * 1000000.0;
    
    // 棰勭儹娴嬭瘯
    string test_sign = SM2_sign(priKey, message, sm2_id, engine);
    if (test_sign.empty()) {
        printf("杩涚▼ %d: 棰勭儹绛惧悕娴嬭瘯澶辫触\n", process_id);
        return;
    }
    
    // 姝ｅ紡鎬ц兘娴嬭瘯
    while (get_current_time_us() < end_time) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            string sign_result = SM2_sign(priKey, message, sm2_id, engine);
            if (!sign_result.empty()) {
                sign_count++;
            }
            
            if (get_current_time_us() >= end_time) {
                break;
            }
        }
    }
    
    double total_time = (get_current_time_us() - start_time) / 1000000.0;
    
    // 鏇存柊鍏变韩鍐呭瓨
    __sync_fetch_and_add(&shared_data->total_sign_ops, sign_count);
    __sync_fetch_and_add(&shared_data->processes_completed, 1);
    shared_data->process_pids[process_id] = getpid();
}

// 楠岀鎬ц兘娴嬭瘯
void run_verify_perf_test(EVP_PKEY* pubKey, const string& message, const string& signMsg, 
                         const string& sm2_id, ENGINE* engine, int process_id) {
    
    long verify_count = 0;
    double start_time = get_current_time_us();
    double end_time = start_time + TEST_DURATION * 1000000.0;
    
    // 棰勭儹娴嬭瘯
    SM2_verify(pubKey, message, signMsg, sm2_id, engine);
    
    // 姝ｅ紡鎬ц兘娴嬭瘯
    while (get_current_time_us() < end_time) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            SM2_verify(pubKey, message, signMsg, sm2_id, engine);
            verify_count++;
            
            if (get_current_time_us() >= end_time) {
                break;
            }
        }
    }
    
    double total_time = (get_current_time_us() - start_time) / 1000000.0;
    
    // 鏇存柊鍏变韩鍐呭瓨
    __sync_fetch_and_add(&shared_data->total_verify_ops, verify_count);
    __sync_fetch_and_add(&shared_data->processes_completed, 1);
    shared_data->process_pids[process_id] = getpid();
    
}

// 鍔犲瘑鎬ц兘娴嬭瘯
void run_encrypt_perf_test(EVP_PKEY* pubKey, const string& message, 
                          ENGINE* engine, int process_id) {
    
    long encrypt_count = 0;
    double start_time = get_current_time_us();
    double end_time = start_time + TEST_DURATION * 1000000.0;
    
    // 棰勭儹娴嬭瘯
    string test_enc = SM2_enc(pubKey, message, engine);
    if (test_enc.empty()) {
        printf("杩涚▼ %d: 棰勭儹鍔犲瘑娴嬭瘯澶辫触\n", process_id);
        return;
    }
    
    // 姝ｅ紡鎬ц兘娴嬭瘯
    while (get_current_time_us() < end_time) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            string enc_result = SM2_enc(pubKey, message, engine);
            if (!enc_result.empty()) {
                encrypt_count++;
            }
            
            if (get_current_time_us() >= end_time) {
                break;
            }
        }
    }
    
    double total_time = (get_current_time_us() - start_time) / 1000000.0;
    
    // 鏇存柊鍏变韩鍐呭瓨
    __sync_fetch_and_add(&shared_data->total_encrypt_ops, encrypt_count);
    __sync_fetch_and_add(&shared_data->processes_completed, 1);
    shared_data->process_pids[process_id] = getpid();
    
}

// 瑙ｅ瘑鎬ц兘娴嬭瘯
void run_decrypt_perf_test(EVP_PKEY* priKey, const string& encMsg, 
                          ENGINE* engine, int process_id) {
    
    long decrypt_count = 0;
    double start_time = get_current_time_us();
    double end_time = start_time + TEST_DURATION * 1000000.0;
    
    // 棰勭儹娴嬭瘯
    string test_dec = SM2_dec(priKey, encMsg, engine);
    if (test_dec.empty()) {
        printf("杩涚▼ %d: 棰勭儹瑙ｅ瘑娴嬭瘯澶辫触\n", process_id);
        return;
    }
    
    // 姝ｅ紡鎬ц兘娴嬭瘯
    while (get_current_time_us() < end_time) {
        for (int i = 0; i < BATCH_SIZE; i++) {
            string dec_result = SM2_dec(priKey, encMsg, engine);
            if (!dec_result.empty()) {
                decrypt_count++;
            }
            
            if (get_current_time_us() >= end_time) {
                break;
            }
        }
    }
    
    double total_time = (get_current_time_us() - start_time) / 1000000.0;
    
    // 鏇存柊鍏变韩鍐呭瓨
    __sync_fetch_and_add(&shared_data->total_decrypt_ops, decrypt_count);
    __sync_fetch_and_add(&shared_data->processes_completed, 1);
    shared_data->process_pids[process_id] = getpid();
}

// 瀛愯繘绋嬫祴璇曞嚱鏁?
void child_process_test(EVP_PKEY* priKey, EVP_PKEY* pubKey, const string& message, 
                       const string& sm2_id, int process_id, const string& test_type) {
    printf("Process %d \n", process_id);
    // 姣忎釜瀛愯繘绋嬬嫭绔嬪垵濮嬪寲KAE寮曟搸
 ENGINE *engine=init_kae_engine();
    
    // 绛夊緟鎵€鏈夎繘绋嬪氨缁?
    while (__sync_fetch_and_add(&shared_data->ready, 0) == 0) {
        usleep(1000);
    }
    
    // 棰勭敓鎴愭祴璇曟暟鎹?
    string enc_data;
    string sign_data;
    
    if (test_type == "decrypt") {
        enc_data = SM2_enc(pubKey, message, engine);
        if (enc_data.empty()) {
            printf("杩涚▼ %d: 鐢熸垚鍔犲瘑鏁版嵁澶辫触\n", process_id);
            cleanup_engine(engine);
            exit(1);
        }
    } else if (test_type == "verify") {
        sign_data = SM2_sign(priKey, message, sm2_id, engine);
        if (sign_data.empty()) {
            printf("杩涚▼ %d: 鐢熸垚绛惧悕鏁版嵁澶辫触\n", process_id);
            cleanup_engine(engine);
            exit(1);
        }
    }
    
    // 鏍规嵁娴嬭瘯绫诲瀷鎵ц涓嶅悓鐨勬祴璇?
    if (test_type == "sign") {
        run_sign_perf_test(priKey, message, sm2_id, engine, process_id);
    } else if (test_type == "verify") {
        run_verify_perf_test(pubKey, message, sign_data, sm2_id, engine, process_id);
    } else if (test_type == "encrypt") {
        run_encrypt_perf_test(pubKey, message, engine, process_id);
    } else if (test_type == "decrypt") {
        run_decrypt_perf_test(priKey, enc_data, engine, process_id);
    }
    
    cleanup_engine(engine);
    exit(0);
}

// 鎵撳嵃鎬诲甫瀹界粨鏋?
void print_total_bandwidth_results(const string& test_type) {
    printf("\n");
    printf("==================================================\n");
    printf("           SM2 %s 澶氳繘绋嬫€诲甫瀹芥祴璇曠粨鏋淺n", test_type.c_str());
    printf("==================================================\n");
    printf("杩涚▼鏁? %d\n", NUM_PROCESSES);
    printf("娴嬭瘯鏃堕暱: %d 绉?杩涚▼\n", TEST_DURATION);
    printf("瀹屾垚杩涚▼鏁? %d/%d\n", shared_data->processes_completed, NUM_PROCESSES);
    printf("寮曟搸: KAE\n");
    printf("--------------------------------------------------\n");
    
    if (test_type == "sign") {
        long total_ops = shared_data->total_sign_ops;
        double total_bandwidth = (double)total_ops / TEST_DURATION;
        printf("鎬荤鍚嶆搷浣滄暟: %ld\n", total_ops);
        printf("鎬荤鍚嶅甫瀹? %.2f ops/s\n", total_bandwidth);
        printf("骞冲潎姣忚繘绋嬪甫瀹? %.2f ops/s\n", total_bandwidth / NUM_PROCESSES);
    } else if (test_type == "verify") {
        long total_ops = shared_data->total_verify_ops;
        double total_bandwidth = (double)total_ops / TEST_DURATION;
        printf("鎬婚獙绛炬搷浣滄暟: %ld\n", total_ops);
        printf("鎬婚獙绛惧甫瀹? %.2f ops/s\n", total_bandwidth);
        printf("骞冲潎姣忚繘绋嬪甫瀹? %.2f ops/s\n", total_bandwidth / NUM_PROCESSES);
    } else if (test_type == "encrypt") {
        long total_ops = shared_data->total_encrypt_ops;
        double total_bandwidth = (double)total_ops / TEST_DURATION;
        printf("鎬诲姞瀵嗘搷浣滄暟: %ld\n", total_ops);
        printf("鎬诲姞瀵嗗甫瀹? %.2f ops/s\n", total_bandwidth);
        printf("骞冲潎姣忚繘绋嬪甫瀹? %.2f ops/s\n", total_bandwidth / NUM_PROCESSES);
    } else if (test_type == "decrypt") {
        long total_ops = shared_data->total_decrypt_ops;
        double total_bandwidth = (double)total_ops / TEST_DURATION;
        printf("鎬昏В瀵嗘搷浣滄暟: %ld\n", total_ops);
        printf("鎬昏В瀵嗗甫瀹? %.2f ops/s\n", total_bandwidth);
        printf("骞冲潎姣忚繘绋嬪甫瀹? %.2f ops/s\n", total_bandwidth / NUM_PROCESSES);
    }
    printf("==================================================\n");
}

// 澶氳繘绋嬫€ц兘娴嬭瘯
void run_multi_process_perf_test(EVP_PKEY* priKey, EVP_PKEY* pubKey, const string& message, 
                                const string& sm2_id, const string& test_type) {
    printf("\n**************************************************\n");
    printf("*           SM2 %s 澶氳繘绋嬫€ц兘娴嬭瘯           *\n", test_type.c_str());
    printf("**************************************************\n");
    printf("杩涚▼鏁? %d\n", NUM_PROCESSES);
    printf("娴嬭瘯鏃堕暱: %d 绉?杩涚▼\n", TEST_DURATION);
    printf("寮曟搸: KAE\n");
    
    // 鍒涘缓瀛愯繘绋?
    for (int i = 0; i < NUM_PROCESSES; i++) {
        pid_t pid = fork();
        
        if (pid == 0) {
            // 瀛愯繘绋?
            child_process_test(priKey, pubKey, message, sm2_id, i, test_type);
        } else if (pid > 0) {
            // 鐖惰繘绋嬭褰昉ID
            shared_data->process_pids[i] = pid;
            usleep(50000); // 50ms寤惰繜閬垮厤璧勬簮绔炰簤
        } else {
            perror("fork");
            exit(1);
        }
    }
    
    // 鍚姩鎵€鏈夎繘绋?
    printf("鍚姩鎵€鏈夎繘绋嬫祴璇?..\n");
    shared_data->ready = 1;
    
    // 绛夊緟鎵€鏈夊瓙杩涚▼瀹屾垚
    for (int i = 0; i < NUM_PROCESSES; i++) {
        int status;
        wait(&status);
    }
    
    printf("\n鎵€鏈夎繘绋嬫祴璇曞畬鎴?\n");
    
    // 鎵撳嵃鎬诲甫瀹界粨鏋?
    print_total_bandwidth_results(test_type);
}

int main(int argc, char* argv[])
{
    // 鍒濆鍖栧叡浜唴瀛?
    if (init_shared_memory() != 0) {
        printf("閿欒: 鍏变韩鍐呭瓨鍒濆鍖栧け璐n");
        return -1;
    }
    
    string prikeyStr, pubkeyStr;
    SM2_GenKey(prikeyStr, pubkeyStr);
    cout << "\n----------------------------TEST SM2---------------------------------------\n";
    cout << "prikey : \n" << prikeyStr << "\n\npubkey: \n" << pubkeyStr << endl;

    EVP_PKEY* evp_priKey = SM2_CreateEVP_PKEY(prikeyStr, false);
    EVP_PKEY* evp_pubKey = SM2_CreateEVP_PKEY(pubkeyStr, true);
    if (!evp_priKey || !evp_pubKey) {
        cout << "evp key is NULL" << endl;
        cleanup_shared_memory();
        return -1;
    }

    OpenSSL_add_all_algorithms();
    ENGINE_load_dynamic();


    const string message("Hello openssl sm2!");
    const string sm2_id("snowdance1997");
    
    cout << "\n---------------------------鍔熻兘娴嬭瘯---------------------------------------\n";
    /* 绛惧悕涓庨獙绛?*/
    string signStr = SM2_sign(evp_priKey, message, sm2_id, NULL);
    if (!signStr.empty()) {
        PrintBufferHex(signStr.c_str(), signStr.size());
        SM2_verify(evp_pubKey, message, signStr, sm2_id, NULL);
    }

    cout << "\n---------------------------鍔犲瘑瑙ｅ瘑娴嬭瘯---------------------------------------\n";
    /* 鍔犲瘑涓庤В瀵?*/
    PrintBufferHex(message.c_str(), message.size());
    string encStr = SM2_enc(evp_pubKey, message, NULL);
    string decStr = SM2_dec(evp_priKey, encStr, NULL);
    PrintBufferHex(decStr.c_str(), decStr.size());
    
    cout << "[message] " << message << endl;
    cout << "[decStr] " << decStr << endl;
    
    if (message == decStr) {
        cout << "[RES] SM2 enc&dec succeeded!" << endl;
    } else {
        cout << "[RES] SM2 enc&dec failed!" << endl;
    }

    // 鎬ц兘娴嬭瘯
    cout << "\n---------------------------鎬ц兘娴嬭瘯---------------------------------------\n";
    
    // 鏍规嵁鍛戒护琛屽弬鏁伴€夋嫨娴嬭瘯绫诲瀷
    string test_type = "sign"; // 榛樿娴嬭瘯绛惧悕
    if (argc > 1) {
        test_type = argv[1];
    }
    
    if (test_type == "sign" || test_type == "all") {
        run_multi_process_perf_test(evp_priKey, evp_pubKey, message, sm2_id, "sign");
    }
    if (test_type == "verify" || test_type == "all") {
        run_multi_process_perf_test(evp_priKey, evp_pubKey, message, sm2_id, "verify");
    }
    if (test_type == "encrypt" || test_type == "all") {
        run_multi_process_perf_test(evp_priKey, evp_pubKey, message, sm2_id, "encrypt");
    }
    if (test_type == "decrypt" || test_type == "all") {
        run_multi_process_perf_test(evp_priKey, evp_pubKey, message, sm2_id, "decrypt");
    }

    // 娓呯悊璧勬簮
    EVP_PKEY_free(evp_priKey);
    EVP_PKEY_free(evp_pubKey);
    cleanup_shared_memory();

    return 0;
}