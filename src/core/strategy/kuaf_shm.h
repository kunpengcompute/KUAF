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
#ifndef KUAF_SHM_H
#define KUAF_SHM_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define BW_GLOBAL_SHM_KEY 0x1234
#define BW_GlOBAL_SEM_KEY 0x4321

#define PARAL_SHM_KEY 0x6789
#define PARAL_SEM_KEY 0x4895

#define SEMAPHORE_INIT_LOCK "/tmp/semaphore_init.lock"

int init_global_semaphore(int key_id);

int init_lock_semaphore(int key_id);

int init_multi_semaphore(int key_id, int value);

void semaphore_P(int semid);

int semaphore_try_P(int semid);

void semaphore_V(int semid);

#endif