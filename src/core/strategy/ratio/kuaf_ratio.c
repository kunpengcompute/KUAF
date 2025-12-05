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
#include <unistd.h>
#include <time.h>
#include <sys/syscall.h>

#include "kuaf_ratio.h"

#define RANDOM_POOL_SIZE 256

static int pool_index = 0;
static int random_pool[RANDOM_POOL_SIZE];

void kuaf_ratio_init_random_pool()
{
    unsigned int seed = (unsigned int)time(NULL) ^ (unsigned int)syscall(SYS_gettid);
    for (int i = 0; i < RANDOM_POOL_SIZE; i++) {
        // 100 means the random number range is 1-100
        random_pool[i] = rand_r(&seed) % 100 + 1;
    }
}

int kuaf_ratio_get_random_value()
{
    if (pool_index >= RANDOM_POOL_SIZE) {
        pool_index = 0;
    }
    return random_pool[(pool_index++) % RANDOM_POOL_SIZE];
}