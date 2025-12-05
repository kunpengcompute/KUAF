/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/shm.h>
#include <sys/time.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <signal.h>
#include <pthread.h>
#include "../../../common/kuaf_config.h"
#include "../../../common/kuaf_log.h"
#include "../kuaf_shm.h"

#include "securec.h"

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

typedef struct ThreadRecord {
    float length;
    struct ThreadRecord *next;
    struct timeval timestamp;
    pid_t tid;
} ThreadRecord;

typedef struct {
    int dev_id;
    int sem;
    float bandwidth;
    ThreadRecord *tl_record_head;
    int share_record_count;
    pthread_spinlock_t spinlock;
} LocalDev;

static int kuaf_shmid;
static volatile int local_dev_count = 0;
static pthread_spinlock_t thread_lock;
static int thread_lock_initialized = 0;
static int bw_expired_interval_ms = 500;
static int bw_update_interval_ms = 50;
static int bw_expired_interval_ms_min_val = 50;
static int bw_update_interval_ms_min_val = 5;

static volatile int global_sem;
static SharedDevList *shared_devs;
static LocalDev local_devs[MAX_DEV_NODE_NUM];

static pthread_t daemon_thread;
static int daemon_start_sleep_min_ratio = 0.1;
static int daemon_start_sleep_max_ratio = 0.3;

long long current_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    // 1000 is used to convert from microseconds to milliseconds
    return (long long)(tv.tv_sec) * 1000 + (tv.tv_usec) / 1000;
}

long long time_diff_ms(struct timeval start, struct timeval end)
{
    // 1000 is used to convert from microseconds to milliseconds
    return (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;
}

int get_dev_sem(int dev_id)
{
    for (int i = 0; i < local_dev_count; i++) {
        if (local_devs[i].dev_id == dev_id) {
            return local_devs[i].sem;
        }
    }
    return -1;
}

void shared_init_dev_records(SharedDev *node)
{
    node->free_head = 1;
    node->head = 0;
    node->tail = 0;
    node->record_count = 0;

    for (int i = 1; i < MAX_RECORDS; i++) {
        node->records[i].prev = 0;
        node->records[i].next = (i < MAX_RECORDS-1) ? (i + 1) : 0;
    }
}

void shared_delete_dev_records_by_idx(SharedDev *node, int index)
{
    if (index <= 0 || index >= MAX_RECORDS || (node->head == node->tail)) {
        return;
    }

    if (index == node->head) {
        node->head = node->records[index].next;
        if (node->head != 0) {
            node->records[node->head].prev = 0;
        } else {
            node->tail = 0;
        }
    } else if (index == node->tail) {
        node->tail = node->records[index].prev;
        if (node->tail != 0) {
            node->records[node->tail].next = 0;
        } else {
            node->head = 0;
        }
    } else {
        int prev = node->records[index].prev;
        int next = node->records[index].next;
        node->records[prev].next = next;
        node->records[next].prev = prev;
    }

    node->record_count--;
    node->records[index].next = node->free_head;
    node->free_head = index;
}

int shared_check_list_is_null(SharedDev *node)
{
    if (node->record_count == 0) {
        return 1;
    }
    return 0;
}

int shared_insert_dev_record(SharedDev *node, struct timeval timestamp, float length, pid_t pid)
{
    int sem = node->sem;

    semaphore_P(sem);
    if (node->free_head == 0) {
        shared_delete_dev_records_by_idx(node, node->head);
    }

    int new_node = node->free_head;
    node->free_head = node->records[new_node].next;

    node->records[new_node].timestamp = timestamp;
    node->records[new_node].length = length;
    node->records[new_node].pid = pid;

    if (node->head == 0) {
        node->head = new_node;
        node->tail = new_node;
        node->records[new_node].prev = 0;
        node->records[new_node].next = 0;
    } else {
        node->records[node->tail].next = new_node;
        node->records[new_node].prev = node->tail;
        node->records[new_node].next = 0;
        node->tail = new_node;
    }

    node->record_count++;
    semaphore_V(sem);

    return new_node;
}

SharedDev *shared_get_or_register_dev_node(int dev_id)
{
    semaphore_P(global_sem);
    for (int i = 0; i < shared_devs->dev_count; i++) {
        if (shared_devs->nodes[i].dev_id == dev_id) {
            semaphore_V(global_sem);
            return &shared_devs->nodes[i];
        }
    }

    if (shared_devs->dev_count < MAX_DEV_NODE_NUM) {
        shared_devs->nodes[shared_devs->dev_count].dev_id = dev_id;
        shared_devs->nodes[shared_devs->dev_count].sem = init_lock_semaphore(dev_id);
        shared_init_dev_records(&(shared_devs->nodes[shared_devs->dev_count]));
        shared_devs->dev_count++;
        semaphore_V(global_sem);
        return &shared_devs->nodes[shared_devs->dev_count - 1];
    }

    semaphore_V(global_sem);
    return NULL;
}

void shared_update_dev_record(SharedDev *node, pid_t pid, struct timeval timestamp, float length)
{
    int sem = node->sem;
    semaphore_P(sem);
    int cur = node->head;
    while (cur != 0) {
        long long diff_ms = time_diff_ms(node->records[cur].timestamp, timestamp);
        if (node->records[cur].pid == pid && diff_ms == 0) {
            node->records[cur].length = length;
            break;
        }
        cur = node->records[cur].next;
    }
    semaphore_V(sem);
}

ThreadRecord *create_thread_record_node()
{
    ThreadRecord *node = (ThreadRecord *)malloc(sizeof(ThreadRecord));
    if (node == NULL) {
        return NULL;
    }
    node->next = NULL;
    return node;
}

struct timeval *add_thread_record_node(
    ThreadRecord *tl_record_head, float length, pid_t tid, pthread_spinlock_t *spinlock)
{
    if (tl_record_head == NULL) {
        return NULL;
    }
    ThreadRecord *cur = tl_record_head->next;
    while (cur != NULL) {
        if (cur->tid == tid) {
            pthread_spin_lock(spinlock);
            cur->length += length;
            pthread_spin_unlock(spinlock);
            return &cur->timestamp;
        }
        cur = cur->next;
    }

    pthread_spin_lock(spinlock);
    ThreadRecord *newNode = create_thread_record_node();
    if (newNode == NULL) {
        pthread_spin_unlock(spinlock);
        return NULL;
    }

    gettimeofday(&newNode->timestamp, NULL);
    newNode->length = length;
    newNode->tid = tid;
    newNode->next = tl_record_head->next;
    tl_record_head->next = newNode;
    pthread_spin_unlock(spinlock);
    return &newNode->timestamp;
}

void free_thread_record_head(ThreadRecord *tl_record_head)
{
    if (!tl_record_head) {
        return;
    }

    ThreadRecord *cur = tl_record_head->next;
    while (cur != NULL) {
        ThreadRecord *temp = cur;
        cur = cur->next;
        free(temp);
    }
    tl_record_head->next = NULL;
}

void init_local_dev_node(int dev_id)
{
    pthread_spin_lock(&thread_lock);
    if (get_dev_sem(dev_id) == -1) {
        if (local_dev_count < MAX_DEV_NODE_NUM) {
            local_devs[local_dev_count].dev_id = dev_id;
            local_devs[local_dev_count].sem = init_lock_semaphore(dev_id);
            local_devs[local_dev_count].tl_record_head = create_thread_record_node();
            pthread_spin_init(&local_devs[local_dev_count].spinlock, PTHREAD_PROCESS_PRIVATE);
            local_dev_count++;
        }
    }
    pthread_spin_unlock(&thread_lock);
}

void upload_bandwidth()
{
    for (int i = 0; i < local_dev_count; i++) {
        pthread_spin_lock(&local_devs[i].spinlock);
        ThreadRecord *cur = local_devs[i].tl_record_head->next;
        ThreadRecord *tmp = cur;
        if (!cur) {
            pthread_spin_unlock(&local_devs[i].spinlock);
            continue;
        }
        local_devs[i].tl_record_head->next = NULL;
        pthread_spin_unlock(&local_devs[i].spinlock);

        int dev_id = local_devs[i].dev_id;
        float total_length = 0;

        while (cur != NULL) {
            total_length += cur->length;
            cur = cur->next;
        }

        free_thread_record_head(tmp);
        free(tmp);

        if (total_length == 0) {
            continue;
        }
        SharedDev *node = shared_get_or_register_dev_node(dev_id);
        if (node == NULL) {
            continue;
        }

        struct timeval now;
        gettimeofday(&now, NULL);

        shared_insert_dev_record(node, now, total_length, getpid());
    }
}

void calculate_bandwidth(int dev_id, long long interval_ms)
{
    SharedDev *node = shared_get_or_register_dev_node(dev_id);

    if (node == NULL) {
        return;
    }

    int sem = node->sem;
    semaphore_P(sem);

    float total_length = 0;
    struct timeval now;
    gettimeofday(&now, NULL);

    for (int i = 0; i < local_dev_count; i++) {
        if (local_devs[i].dev_id == dev_id) {
            int cur = node->head;
            while (cur != 0) {
                long long diff = time_diff_ms(node->records[cur].timestamp, now);
                if (diff <= interval_ms) {
                    total_length += node->records[cur].length;
                }
                cur = node->records[cur].next;
            }

            local_devs[i].bandwidth = total_length;
            local_devs[i].share_record_count = node->record_count;
        }
    }
    semaphore_V(sem);
}

void cleanup_shared_old_records(int dev_id, long long interval_ms)
{
    SharedDev *node = shared_get_or_register_dev_node(dev_id);
    if (node == NULL) {
        return;
    }

    struct timeval now;
    gettimeofday(&now, NULL);
    int cur = node->head;
    int sem = node->sem;

    semaphore_P(sem);
    while (cur != 0) {
        long long diff = time_diff_ms(node->records[cur].timestamp, now);
        int next_node = node->records[cur].next;
        if (diff >= interval_ms) {
            shared_delete_dev_records_by_idx(node, cur);
        }

        if (cur == next_node || shared_check_list_is_null(node)) {
            shared_init_dev_records(node);
            break;
        }
        cur = next_node;
    }
    semaphore_V(sem);
}

void kuaf_bw_update()
{
    for (int i = 0; i < shared_devs->dev_count; i++) {
        cleanup_shared_old_records(shared_devs->nodes[i].dev_id, bw_expired_interval_ms);
        calculate_bandwidth(shared_devs->nodes[i].dev_id, bw_expired_interval_ms);
    }

    upload_bandwidth();
}

void *daemon_thread_func(void *arg)
{
    int min_sleep_time_ms = bw_expired_interval_ms * daemon_start_sleep_min_ratio + 1;
    int max_sleep_time_ms = bw_expired_interval_ms * daemon_start_sleep_max_ratio + 1;
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)syscall(SYS_gettid);
    int rand_val = rand_r(&seed) % (max_sleep_time_ms - min_sleep_time_ms + 1) + min_sleep_time_ms;
    usleep(1000 * rand_val);

    while (1) {
        kuaf_bw_update();
        // 1000 is used to convert from microseconds to milliseconds
        usleep(1000 * bw_update_interval_ms);  // 100ms kuaf_config_get_str
    }
    return NULL;
}

int kuaf_bw_init_async()
{
    struct shmid_ds temp_shmds;
    global_sem = init_global_semaphore(BW_GlOBAL_SEM_KEY);
    if (global_sem == -1) {
        KUAF_ERROR("init semaphore failed");
        return 0;
    }
    semaphore_P(global_sem);
    // 0666 is used to set the permission of shared memory
    kuaf_shmid = shmget(BW_GLOBAL_SHM_KEY, sizeof(SharedDevList), IPC_CREAT | 0666);
    if (kuaf_shmid < 0) {
        KUAF_ERROR("create bandwidth shared memory failed");
        goto cleanup_sem;
    }

    shared_devs = (SharedDevList *)shmat(kuaf_shmid, NULL, 0);
    if (shared_devs == (void *)-1) {
        KUAF_ERROR("attach bandwidth shared memory failed");
        goto cleanup_shm;
    }
    
    struct shmid_ds shmds;
    shmctl(kuaf_shmid, IPC_STAT, &shmds);
    if (shmds.shm_nattch == 1) {
        errno_t ret = memset_s(shared_devs, sizeof(SharedDevList), 0, sizeof(SharedDevList));
        if (ret != EOK) {
            KUAF_ERROR("shared memory init failed");
            goto cleanup_shmat;
        }
    }

    int expired_interval_ms =
        kuaf_config_get_int(KUAF_CONFIG_FILE_NAME, KUAF_CONFIG_SECTION_NAME, KUAF_CONFIG_BW_EXPIRED_INTERVAL);
    int update_interval_ms =
        kuaf_config_get_int(KUAF_CONFIG_FILE_NAME, KUAF_CONFIG_SECTION_NAME, KUAF_CONFIG_BW_UPDATE_INTERVAL);
    if (expired_interval_ms >= bw_expired_interval_ms_min_val) {
        bw_expired_interval_ms = expired_interval_ms;
    }
    if (update_interval_ms >= bw_update_interval_ms_min_val) {
        bw_update_interval_ms = update_interval_ms;
    }
    if (pthread_spin_init(&thread_lock, PTHREAD_PROCESS_PRIVATE) != 0) {
        KUAF_ERROR("init thread lock failed");
        goto cleanup_shmat;
    }
    thread_lock_initialized = 1;
    if (pthread_create(&daemon_thread, NULL, daemon_thread_func, NULL) != 0) {
        KUAF_ERROR("create daemon thread failed");
        goto cleanup_spinlock;
    }
    if (pthread_detach(daemon_thread) != 0) {
        KUAF_ERROR("detach daemon thread failed");
        goto cleanup_thread;
    }
    semaphore_V(global_sem);
    return 1;

cleanup_thread:
    pthread_cancel(daemon_thread);
cleanup_spinlock:
    pthread_spin_destroy(&thread_lock);
    thread_lock_initialized = 0;
cleanup_shmat:
    shmdt(shared_devs);
    shared_devs = NULL;
cleanup_shm:
    if (shmctl(kuaf_shmid, IPC_STAT, &temp_shmds) != -1 && temp_shmds.shm_nattch <= 1) {
        shmctl(kuaf_shmid, IPC_RMID, NULL);
    }
    kuaf_shmid = -1;
cleanup_sem:
    semaphore_V(global_sem);
    return 0;
}

struct timeval *kuaf_bw_record(int dev_id, float length)
{
    int find_flag = 0;
    for (int i = 0; i < local_dev_count; i++) {
        if (local_devs[i].dev_id == dev_id) {
            find_flag = 1;
            break;
        }
    }
    if (!find_flag) {
        init_local_dev_node(dev_id);
    }

    pid_t tid = syscall(SYS_gettid);
    for (int i = 0; i < local_dev_count; i++) {
        if (local_devs[i].dev_id == dev_id) {
            return add_thread_record_node(local_devs[i].tl_record_head, length, tid, &local_devs[i].spinlock);
        }
    }
    return NULL;
}

float kuaf_bw_get(int dev_id)
{
    int find_flag = 0;
    for (int i = 0; i < local_dev_count; i++) {
        if (local_devs[i].dev_id == dev_id) {
            find_flag = 1;
            break;
        }
    }
    if (!find_flag) {
        init_local_dev_node(dev_id);
        calculate_bandwidth(dev_id, bw_expired_interval_ms);
    }

    for (int i = 0; i < local_dev_count; i++) {
        if (local_devs[i].dev_id == dev_id) {
            float bw = local_devs[i].bandwidth / (bw_expired_interval_ms / 1000.0);
            return bw;
        }
    }
    return -1;
}

void kuaf_bw_free_async()
{
    if (daemon_thread) {
        int cancel_ret = pthread_cancel(daemon_thread);
        if (cancel_ret == 0) {
            KUAF_DEBUG("Cancellation request sent to detached thread.");
            daemon_thread = 0;
        } else if (cancel_ret == ESRCH) {
            KUAF_ERROR("Thread already terminated or invalid thread ID.");
            daemon_thread = 0;
        } else {
            KUAF_ERROR("Failed to cancel detached thread (error %d: %s)", 
                cancel_ret, strerror(cancel_ret));
        }
    }
    if (global_sem == -1) {
        return;
    }
    semaphore_P(global_sem);
    if (kuaf_shmid >= 0 && shared_devs && shared_devs != (void *)-1) {
        struct shmid_ds shmds;
        if (shmctl(kuaf_shmid, IPC_STAT, &shmds) != -1) {
            if (shmds.shm_nattch == 1) {
                for (int i = 0; i < shared_devs->dev_count && i < MAX_DEV_NODE_NUM; i++) {
                    int sem = shared_devs->nodes[i].sem;
                    if (sem != -1) {
                        semctl(sem, 0, IPC_RMID);
                    }
                }
            }
        } else {
            KUAF_ERROR("Failed to get shared memory info: %s", strerror(errno));
        }
        shmdt(shared_devs);
        shared_devs = NULL;
        if (shmctl(kuaf_shmid, IPC_STAT, &shmds) != -1 && shmds.shm_nattch == 0) {
            shmctl(kuaf_shmid, IPC_RMID, NULL);
        }
        kuaf_shmid = -1;
    }
    semaphore_V(global_sem);

    for (int i = 0; i < local_dev_count; i++) {
        pthread_spin_destroy(&local_devs[i].spinlock);
        if (!local_devs[i].tl_record_head) {
            continue;
        }
        free_thread_record_head(local_devs[i].tl_record_head);
        free(local_devs[i].tl_record_head);
        local_devs[i].tl_record_head = NULL;
    }
    if (thread_lock_initialized) {
        pthread_spin_destroy(&thread_lock);
        thread_lock_initialized = 0;
    }
}