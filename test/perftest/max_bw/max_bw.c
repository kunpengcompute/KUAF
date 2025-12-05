#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <stdio.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE 256
#define BW_GLOBAL_SHM_KEY 0x1234
#define BW_GlOBAL_SEM_KEY 0x4321
#define DEV_NUM 20

#define MAX_DEV_NODE_NUM 16
#define MAX_RECORDS 1024

typedef struct {
    struct timeval timestamp;
    float length;
    pid_t pid;
    int prev;
    int next;
} BandwidthRecord;

typedef struct {
    int dev_id;
    int record_count;
    BandwidthRecord records[MAX_RECORDS];
    int free_head;
    int head;
    int tail;
    int sem;
} SharedDev;

typedef struct {
    int dev_count;
    SharedDev nodes[MAX_DEV_NODE_NUM];
} SharedDevList;

typedef enum {
    DEV_ID_UNKNOWN,
    DEV_HPRE_0,
    DEV_HPRE_1,
    DEV_HPRE_2,
    DEV_HPRE_3,
    DEV_SEC_0,
    DEV_SEC_1,
    DEV_SEC_2,
    DEV_SEC_3,
    DEV_ZIP_0,
    DEV_ZIP_1,
    DEV_ZIP_2,
    DEV_ZIP_3,
    DEV_ID_MAX_SIZE
} KUAF_DEV_ID;

typedef struct {
    int id;
    char *name;
} AlgMapEntry;

AlgMapEntry alg_map[] = {{DEV_HPRE_0, "hpre-0"},
    {DEV_HPRE_1, "hpre-1"},
    {DEV_HPRE_2, "hpre-2"},
    {DEV_HPRE_3, "hpre-3"},

    {DEV_SEC_0, "sec-0"},
    {DEV_SEC_1, "sec-1"},
    {DEV_SEC_2, "sec-2"},
    {DEV_SEC_3, "sec-3"},

    {DEV_ZIP_0, "zip-0"},
    {DEV_ZIP_1, "zip-1"},
    {DEV_ZIP_2, "zip-2"},
    {DEV_ZIP_3, "zip-3"},

    {0, NULL}
};

typedef struct {
    char* alg_name;
    int bw;
} BwMapEntry;

BwMapEntry bw_map[] = {{"zlib-deflate-v1", 8192},
    {"zlib-inflate-v1", 6144},
    {"zlib-deflate-v2", 4096},
    {"zlib-inflate-v2", 3072},
    {NULL, 0}
};

char *get_dev_name_by_id(int alg_id)
{
    for (int i = 0; alg_map[i].name != NULL; i++) {
        if (alg_map[i].id == alg_id) {
            return alg_map[i].name;
        }
    }
    return NULL;
}

int get_alg_max_bw_by_name(char *alg_name)
{
    for (int i = 0; bw_map[i].alg_name != NULL; i++) {
        if (strcmp(bw_map[i].alg_name, alg_name) == 0) {
            return bw_map[i].bw;
        }
    }
    return -1;
}

char* get_cpu_type()
{
    unsigned long long cpuId;
    __asm__ volatile("mrs %0, MIDR_EL1" : "=r"(cpuId));

    unsigned long long vendor = (cpuId >> 0x18) & 0xFF;
    unsigned long long partId = (cpuId >> 0x4) & 0xFFF;
    if ((vendor == 0x48) && (partId == 0xD01)) {
        return "-v1";
    } else if ((vendor == 0x48) && (partId == 0xD02)) {
        return "-v2";
    } else if ((vendor == 0x48) && (partId == 0xD03)) {
        return "-v3";
    } else if (partId == 0xD22) {
        return "-v4";
    }
    return NULL;
}

// P鎿嶄綔锛堢瓑寰咃級
void semaphore_P(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    if (semop(semid, &sops, 1) == -1) {
        printf("semop P operation failed\n");
    }
}

// V鎿嶄綔锛堥噴鏀撅級
void semaphore_V(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    if (semop(semid, &sops, 1) == -1) {
        printf("semop V operation failed\n");
    }
}

/* 鍘绘帀琛屽熬鎹㈣鍜屽洖杞?*/
static void trim_newline(char *s)
{
    char *p = s + strlen(s);
    while (p > s && (p[-1] == '\n' || p[-1] == '\r'))
        --p;
    *p = '\0';
}

/* 鍘绘帀宸﹀彸绌虹櫧锛岃繑鍥炲墺绂诲悗璧峰鎸囬拡 */
static char *trim_ws(char *s)
{
    while (isspace((unsigned char)*s))
        ++s;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)end[-1]))
        --end;
    *end = '\0';
    return s;
}

/* 璇诲彇 [KUAF_Section] 鐨?expired_interval; 鎴愬姛杩斿洖 0 骞跺啓鍏?*out */
int read_expired_interval(const char *filename, int *out)
{
    FILE *fp = fopen(filename, "r");
    if (!fp)
        return -1;

    char line[MAX_LINE];
    int in_section = 0;

    while (fgets(line, sizeof(line), fp)) {
        trim_newline(line);
        char *l = trim_ws(line);

        /* 璺宠繃绌鸿鍜屾敞閲?*/
        if (*l == '\0' || *l == ';' || *l == '#')
            continue;

        /* 鑺傚悕琛?*/
        if (*l == '[') {
            in_section = (strncmp(l, "[KUAF_Section]", 15) == 0);
            continue;
        }

        if (!in_section)
            continue;

        /* 閸?鍊?*/
        char *eq = strchr(l, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char *key = trim_ws(l);
        char *val = trim_ws(eq + 1);

        if (strcmp(key, "expired_interval") == 0) {
            *out = atoi(val);
            fclose(fp);
            return 0; /* 鎴愬姛 */
        }
    }

    fclose(fp);
    return -2; /* 鎵句笉鍒伴敭 */
}

int main(int argc, char **argv)
{
    const char *optstring = "t:x:";
    int o = 0;
    int time_seconds = 5;
    float max_bws[DEV_NUM] = {0};
    int find = 0;
    int expired_interval;
    char alg_name[32] = {0};

    SharedDevList *shared_devs;
    struct shmid_ds shmds;
    char* cpu_type =  get_cpu_type();
    if (!cpu_type) {
        printf("Kuaf unsupport cpu type.\n");
        return 0;
    }

    while ((o = getopt(argc, argv, optstring)) != -1) {
        if (optstring == NULL)
            continue;
        switch (o) {
            case 't':
                time_seconds = atoi(optarg);
                break;
            case 'x':
                strcpy(alg_name, optarg);
                strcat(alg_name, cpu_type);
                break;
            default:
                break;
        }
    }

    if (get_alg_max_bw_by_name(alg_name) == -1) {
        printf("Kuaf unsupport alg type.\n");
        printf("Please specify it with -x.\n");
        return 0;
    }

    printf("Sampling time is: %ds\n", time_seconds);

    int kuaf_shmid = shmget(BW_GLOBAL_SHM_KEY, sizeof(SharedDevList), IPC_CREAT | 0666);
    if (kuaf_shmid < 0) {
        printf("Create bw shared memory failed.\n");
    }
    
    shared_devs = (SharedDevList *)shmat(kuaf_shmid, NULL, 0);
    if (shared_devs == (void *)-1) {
        printf("Attach bw shared memory failed.\n");
    }

    shmctl(kuaf_shmid, IPC_STAT, &shmds);
    if (shmds.shm_nattch == 1) {
        printf("No dev running with kuaf.\n");
        return 0;
    }

    for (float i = 0.3; i < time_seconds; i += 0.3) {
        for (int j = 0; j < shared_devs->dev_count; j++) {
            int sem = shared_devs->nodes[j].sem;
            semaphore_P(sem);
            SharedDev *node = &shared_devs->nodes[j];
            if (node == NULL) {
                continue;
            }
            float total_length = 0;

            int cur = node->head;
            while (cur != 0) {
                total_length += node->records[cur].length;
                cur = node->records[cur].next;
            }
            if (total_length > max_bws[node->dev_id]) {
                max_bws[node->dev_id] = total_length;
            }
            semaphore_V(sem);
        }
        usleep(1000 * 300);
    }

    if (read_expired_interval("/etc/kuaf/kuaf.cnf", &expired_interval) == -1) {
        printf("Not found /etc/kuaf/kuaf.cnf.\n");
        return 0;
    }

    for (int i = 0; i < DEV_NUM; i++) {
        if (max_bws[i] != 0) {
            find = 1;
            int max_bw = (int)(max_bws[i] / (expired_interval / 1000.0) - 75);
            if (strcmp("zlib-deflate-v1", alg_name) == 0 || strcmp("zlib-deflate-v2", alg_name) == 0) {
                max_bw *= 0.954;
            }
            if (max_bw < get_alg_max_bw_by_name(alg_name)) {
                printf("%s Suggest bw: %d\n", get_dev_name_by_id(i), max_bw);
            }else {
                printf("%s Suggest bw: %d\n", get_dev_name_by_id(i), get_alg_max_bw_by_name(alg_name));
            }
        }
    }

    if (!find) {
        printf("All dev bw is: 0\n");
    }

    shmctl(kuaf_shmid, IPC_STAT, &shmds);
    if (shmds.shm_nattch == 0) {
        shmctl(kuaf_shmid, IPC_RMID, NULL);
    }

    return 0;
}