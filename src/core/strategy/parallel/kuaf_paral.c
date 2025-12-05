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
#include <stdatomic.h>
#include "../../device/kuaf_dev.h"
#include "../../../common/kuaf_log.h"
#include "../../../common/kuaf_config.h"
#include "../kuaf_shm.h"
#include "kuaf_paral.h"
#include "../../algorithm/kuaf_alg.h"

typedef struct {
    ALG_ID id;
    int is_init;
} AlgParallelInitMap;

static AlgParallelInitMap init_map[] = {
    {ALG_ZLIB_DEFLATE, 0}, {ALG_ZLIB_INFLATE, 0}, {ALG_RSA, 0}, {ALG_SM2, 0}, {ALG_ID_UNKNOWN, -1}};

typedef struct ParallelMap {
    int paral_id;
    int sem;
    struct ParallelMap *next;
} ParallelMap;

static ParallelMap *paral_head;
static int paral_sem;
static int paral_shmid;
static int* shared_paral;

int kuaf_paral_alg_is_init(ALG_ID id)
{
    for (int i = 0; init_map[i].id != ALG_ID_UNKNOWN; i++) {
        if (init_map[i].id == id) {
            return init_map[i].is_init;
        }
    }
    // 如果未找到，返回一个默认值，例如 -1 表示未定义
    return -1;
}

ParallelMap *kuaf_paral_map_init()
{
    ParallelMap *head = (ParallelMap *)malloc(sizeof(ParallelMap));
    if (!head) {
        perror("malloc failed");
        return NULL;
    }
    head->paral_id = -1;
    head->sem = 0;
    head->next = NULL;
    return head;
}

void kuaf_paral_map_insert(ParallelMap *head, int paral_id, int sem)
{
    ParallelMap *node = (ParallelMap *)malloc(sizeof(ParallelMap));
    if (!node) {
        perror("malloc failed");
        return;
    }
    node->paral_id = paral_id;
    node->sem = sem;
    node->next = head->next;
    head->next = node;
}

int kuaf_paral_map_get_sem(ParallelMap *head, int paral_id)
{
    for (ParallelMap *cur = head->next; cur != NULL; cur = cur->next) {
        if (cur->paral_id == paral_id) {
            return cur->sem;
        }
    } 
    return -1;  // 表示没找到
}

// 删除整个链表（释放内存，包括头结点）
void kuaf_paral_map_destroy(ParallelMap *head, int last_process)
{
    ParallelMap *cur = head;
    while (cur) {
        ParallelMap *tmp = cur;
        cur = cur->next;
        if (last_process && tmp->sem != -1) {
            semctl(tmp->sem, 0, IPC_RMID);
        }
        // semctl(cur->sem, 0, IPC_RMID); // 最后一个进程释放 or 系统自动释放,但不会删除。。
        free(tmp);
    }
}

int kuaf_paral_get_id(int alg_id, int offset)
{
    int paral_id = alg_id * DEV_ID_MAX_SIZE + offset + DEV_ID_MAX_SIZE;
    return paral_id;
}

static void kuaf_paral_init_sem(ALG_ID alg_id, int paral_num)
{
    if (paral_num < 0) {
        return;
    }
    ARCH_TYPE cpu_type = kuaf_dev_get_cpu_type();
    int dev_count;

    switch (cpu_type) {
        case CPU_HISI_V1:
            dev_count = 2;
            break;
        case CPU_HISI_V2:
            dev_count = 4;
            break;
        default:
            KUAF_ERROR("init paral faild. unsupport cpu type.\n");
            return;
    }
    int dev_id = ((kuaf_dev_get_id(alg_id) - 1) / KUAF_DEV_MAX_NUM) * KUAF_DEV_MAX_NUM + 1;
    for (int i = 0; i < dev_count; i++) {
        int paral_id = kuaf_paral_get_id(alg_id, dev_id + i);
        int sem = init_multi_semaphore(paral_id, paral_num);
        kuaf_paral_map_insert(paral_head, paral_id, sem);
    }
}

int kuaf_paral_init()
{
    struct shmid_ds temp_shmds;
    paral_sem = init_global_semaphore(PARAL_SEM_KEY);
    if (paral_sem == -1) {
        KUAF_ERROR("init semaphore failed");
        return 0;
    }
    semaphore_P(paral_sem);
    // 0666 is the permission of the shared memory
    paral_shmid = shmget(PARAL_SHM_KEY, sizeof(int), IPC_CREAT | 0666);
    if (paral_shmid < 0) {
        KUAF_ERROR("create paral shared memory failed");
        goto cleanup_sem;
    }

    shared_paral = (int *)shmat(paral_shmid, NULL, 0);
    if (shared_paral == (void *)-1) {
        KUAF_ERROR("attach paral shared memory failed");
        goto cleanup_shm;
    }

    struct shmid_ds shmds;
    shmctl(paral_shmid, IPC_STAT, &shmds);

    paral_head = kuaf_paral_map_init();
    if (paral_head == NULL) {
        KUAF_ERROR("parallel map init failed");
        goto cleanup_shmat;
    }
    for (int i = 0; init_map[i].id != ALG_ID_UNKNOWN; i++) {
        int paral_num = kuaf_config_get_int(
            KUAF_CONFIG_KAE_FILE_NAME, kuaf_alg_get_name(init_map[i].id), KUAF_CONFIG_SCHEDULER_PARALLEL);
        if (paral_num < 1) {
            continue;
        }
        kuaf_paral_init_sem(init_map[i].id, paral_num);
        init_map[i].is_init = 1;
    }
    semaphore_V(paral_sem);
    return 1;

cleanup_shmat:
    // 去除共享内存关联 
    shmdt(shared_paral);
    shared_paral = NULL;
cleanup_shm:
    // 在没有其他进程使用时删除共享内存
    if (shmctl(paral_shmid, IPC_STAT, &temp_shmds) != -1 && temp_shmds.shm_nattch <= 1) {
        shmctl(paral_shmid, IPC_RMID, NULL);
    }
    paral_shmid = -1;
cleanup_sem:
    semaphore_V(paral_sem);
    return 0;
}

int kuaf_paral_try_P(int alg_id, int dev_id)
{
    int paral_id = kuaf_paral_get_id(alg_id, dev_id);
    int sem = kuaf_paral_map_get_sem(paral_head, paral_id);
    if (sem == -1) {
        return -1;
    }

    return semaphore_try_P(sem);
}

void kuaf_paral_V(int alg_id, int dev_id)
{

    int paral_id = kuaf_paral_get_id(alg_id, dev_id);
    int sem = kuaf_paral_map_get_sem(paral_head, paral_id);
    if (sem == -1) {
        return;
    }

    semaphore_V(sem);
}

void kuaf_paral_free()
{
    // 如果信号量初始化失败，说明其他资源必然没有初始化
    if (paral_sem == -1) {
        return;
    }

    semaphore_P(paral_sem);
    if (shared_paral && shared_paral != (void *)-1) {
        shmdt(shared_paral);
        shared_paral = NULL;
    }

    if (paral_shmid >= 0) {
        struct shmid_ds shmds;
        if (shmctl(paral_shmid, IPC_STAT, &shmds) != -1) {
            if (shmds.shm_nattch == 0) {
                shmctl(paral_shmid, IPC_RMID, NULL);
                kuaf_paral_map_destroy(paral_head, 1);
                paral_head = NULL;
            } else {
                kuaf_paral_map_destroy(paral_head, 0);
                paral_head = NULL;
            }
        } else {
            KUAF_ERROR("get paral shared memory info failed");
            kuaf_paral_map_destroy(paral_head, 0);
            paral_head = NULL;
        }
    }
    semaphore_V(paral_sem);
}
