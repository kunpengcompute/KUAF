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
#ifndef KUAF_DEV_H
#define KUAF_DEV_H

typedef enum {
    DEV_MODE_UNKNOWN,
    DEV_NOSVA,
    DEV_SVA
} KUAF_DEV_MODE;

typedef enum {
    DEV_TYPE_UNKNOWN,
    DEV_HPRE,
    DEV_SEC,
    DEV_ZIP
} KUAF_DEV_TYPE;

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

typedef enum ARCH_TYPE {
    CPU_TYPE_UNKNOWN,
    CPU_HISI_V1,
    CPU_HISI_V2,
    CPU_HISI_V3,
    CPU_HISI_V4
} ARCH_TYPE;

#define KUAF_DEV_MAX_NUM 4

void kuaf_dev_init();

int kuaf_dev_get_id(int alg_id);

ARCH_TYPE kuaf_dev_get_cpu_type();

int kuaf_dev_check_id(int dev_id);

int kuaf_dev_check_hard_id(int dev_id);

#endif