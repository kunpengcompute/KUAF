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
#define _GNU_SOURCE

#include <numa.h>
#include <numaif.h>
#include <sched.h>
#include <uadk/wd.h>
#include <uadk/v1/wd.h>
#include "../algorithm/kuaf_alg.h"
#include "../../common/kuaf_log.h"
#include "kuaf_dev.h"

/* DEVICE Module API function */

static ARCH_TYPE arch_type = CPU_TYPE_UNKNOWN;

void kuaf_dev_init()
{
    unsigned long long cpuId;
    __asm__ volatile("mrs %0, MIDR_EL1" : "=r"(cpuId));

    unsigned long long vendor = (cpuId >> 0x18) & 0xFF;
    unsigned long long partId = (cpuId >> 0x4) & 0xFFF;
    if ((vendor == 0x48) && (partId == 0xD01)) {
         arch_type = CPU_HISI_V1;
    } else if ((vendor == 0x48) && (partId == 0xD02)) {
         arch_type = CPU_HISI_V2;
    } else if ((vendor == 0x48) && (partId == 0xD03)) {
         arch_type = CPU_HISI_V3;
    } else if (partId == 0xD22) {
         arch_type = CPU_HISI_V4;
    } else {
         arch_type = CPU_TYPE_UNKNOWN;
    }
}

ARCH_TYPE kuaf_dev_get_cpu_type()
{
    return arch_type;
}

static int get_dev_id_by_type(KUAF_DEV_TYPE dev_type)
{
    int numa_id = numa_node_of_cpu(sched_getcpu());

    if (arch_type == CPU_HISI_V1) {
        // 4 is the max number of numa nodes
        if (numa_id >= 0 && numa_id < 4) {
            int index = numa_id / 2;
            switch (dev_type) {
                case DEV_HPRE:
                    return DEV_HPRE_0 + index;
                case DEV_SEC:
                    return DEV_SEC_0 + index;
                case DEV_ZIP:
                    return DEV_ZIP_0 + index;
                case DEV_TYPE_UNKNOWN:
                    return DEV_ID_UNKNOWN;
            }
        }
    } else if (arch_type == CPU_HISI_V2) {
        // 4 is the max number of numa nodes
        if (numa_id >= 0 && numa_id < 4) {
            switch (dev_type) {
                case DEV_HPRE:
                    return DEV_HPRE_0 + numa_id;
                case DEV_SEC:
                    return DEV_SEC_0 + numa_id;
                case DEV_ZIP:
                    return DEV_ZIP_0 + numa_id;
                case DEV_TYPE_UNKNOWN:
                    return DEV_ID_UNKNOWN;
            }
        }
    }

    return DEV_ID_UNKNOWN;
}

int kuaf_dev_get_id(int alg_id)
{
    switch (alg_id) {
        case ALG_ZLIB_DEFLATE:
        case ALG_ZLIB_INFLATE:
            return get_dev_id_by_type(DEV_ZIP);
        case ALG_RSA:
            return get_dev_id_by_type(DEV_HPRE);
        case ALG_SM2:
            return get_dev_id_by_type(DEV_SEC);
        default:
            return DEV_ID_UNKNOWN;
    }
}

int kuaf_dev_get_rsa_device()
{
    return DEV_NOSVA;
}

int kuaf_dev_check_id(int dev_id)
{
    if (dev_id < DEV_ID_UNKNOWN || dev_id >= DEV_ID_MAX_SIZE) {
        KUAF_ERROR("invalid dev id: %d.", dev_id);
        return 0;
    }
    return 1;
}

int kuaf_dev_check_hard_id(int dev_id)
{
    if (dev_id <= DEV_ID_UNKNOWN || dev_id >= DEV_ID_MAX_SIZE) {
        KUAF_ERROR("invalid hard dev id: %d.", dev_id);
        return 0;
    }
    return 1;
}