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
#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>

#include "../../common/kuaf_log.h"
#include "../device/kuaf_dev.h"
#include "kuaf_shm.h"

// 初始化信号量
int init_global_semaphore(int key_id)
{
    int lock_fd = -1;
    int semid = -1;

    // 获取全局初始化锁（阻塞等待，直到成功）
    lock_fd = open(SEMAPHORE_INIT_LOCK, O_RDWR | O_CREAT, 0644);
    if (lock_fd == -1) {
        KUAF_ERROR("Failed to open lock file");
        return -1;
    }
    if (flock(lock_fd, LOCK_EX) == -1) {
        KUAF_ERROR("Failed to acquire lock");
        close(lock_fd);
        return -1;
    }
    KUAF_DEBUG("Acquired global initialization lock");

    key_t key = (key_t)key_id;
    // 0666 is used to set the permissions of the semaphore set
    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        KUAF_DEBUG("Semaphore set already exists.");
        // 0666 is used to set the permissions of the semaphore set
        semid = semget(key, 1, 0666);
    } else {
        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        };
        union semun arg;
        arg.val = 1;
        if (semctl(semid, 0, SETVAL, arg) == -1) {
            KUAF_ERROR("Semaphore set init failed");
            semctl(semid, 0, IPC_RMID); // 初始化失败，删除信号量
            semid = -1;
            goto cleanup;
        }
        KUAF_DEBUG("Semaphore set init success");
    }

cleanup:
    // 释放锁
    if (lock_fd != -1) {
        flock(lock_fd, LOCK_UN);
        close(lock_fd);
        KUAF_DEBUG("Released global initialization lock");
    }

    return semid;
}

// 初始化信号量
int init_lock_semaphore(int key_id)
{
    int semid = -1;
    key_t key = (key_t)key_id;
    // 0666 is used to set the permissions of the semaphore set
    semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);
    if (semid == -1) {
        KUAF_DEBUG("Semaphore set already exists.");
        // 0666 is used to set the permissions of the semaphore set
        semid = semget(key, 1, 0666);
    } else {
        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        };
        union semun arg;
        arg.val = 1;
        if (semctl(semid, 0, SETVAL, arg) == -1) {
            KUAF_ERROR("Semaphore set init failed");
            semctl(semid, 0, IPC_RMID); // 初始化失败，删除信号量
            semid = -1;
        }
        KUAF_DEBUG("Semaphore set init success");
    }
    return semid;
}

// 初始化信号量
int init_multi_semaphore(int key_id, int value)
{
    key_t key = (key_t)key_id + DEV_ID_MAX_SIZE;
    int semid = semget(key, 1, IPC_CREAT | IPC_EXCL | 0666);

    if (semid == -1) {
        KUAF_DEBUG("Semaphore set already exists.");
        semid = semget(key, 1, 0666);
    } else {
        union semun {
            int val;
            struct semid_ds *buf;
            unsigned short *array;
        };
        union semun arg;
        arg.val = value;

        if (semctl(semid, 0, SETVAL, arg) == -1) {
            KUAF_ERROR("Semaphore set init failed");
            semctl(semid, 0, IPC_RMID); // 初始化失败，删除信号量
            return -1;
        }
        KUAF_DEBUG("Semaphore set init success");
    }

    return semid;
}

// P操作（等待）
void semaphore_P(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO;
    if (unlikely(semop(semid, &sops, 1) == -1)) {
        KUAF_ERROR("semop P operation failed");
    }
}

// P操作（非阻塞）
int semaphore_try_P(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = -1;
    sops.sem_flg = SEM_UNDO | IPC_NOWAIT;
    if (unlikely(semop(semid, &sops, 1) == -1)) {
        if (errno == EAGAIN) {
            return 0;
        } else {
            perror("semop P operation failed");
            return -1;
        }
    }
    return 1;
}

// V操作（释放）
void semaphore_V(int semid)
{
    struct sembuf sops;
    sops.sem_num = 0;
    sops.sem_op = 1;
    sops.sem_flg = SEM_UNDO;
    if (unlikely(semop(semid, &sops, 1) == -1)) {
        KUAF_ERROR("semop V operation failed");
    }
}