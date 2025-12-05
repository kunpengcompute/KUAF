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
#ifndef KUAF_ALG_H
#define KUAF_ALG_H

#include "comp/kuaf_zlib.h"
#include "crypto/kuaf_rsa.h"

typedef enum {
    ALG_TYPE_UNKNOWN = 0,
    ALG_COMP,
    ALG_CRYPTO
} ALG_TYPE;

typedef enum {
    ALG_ID_UNKNOWN = 0,
    ALG_ZLIB_DEFLATE = 1,
    ALG_ZLIB_INFLATE = 2,
    ALG_RSA = 19,
    ALG_SM2 = 20
} ALG_ID;

// 定义ALG_ID与ALG_TYPE的映射关系
typedef struct {
    ALG_ID id;
    ALG_TYPE type;
} AlgIdTypeMapEntry;

static const AlgIdTypeMapEntry id_type_map[] = {
    {ALG_ZLIB_DEFLATE, ALG_COMP},
    {ALG_ZLIB_INFLATE, ALG_COMP},
    {ALG_RSA, ALG_CRYPTO},
    {ALG_SM2, ALG_CRYPTO}
};

typedef struct {
    int id;
    char *name;
} AlgMapEntry;

static const AlgMapEntry alg_map[] = {
    {ALG_ZLIB_DEFLATE, "ZLIB-DEFLATE"},
    {ALG_ZLIB_INFLATE, "ZLIB-INFLATE"},
    {ALG_RSA, "RSA"},
    {ALG_SM2, "SM2"},
    {ALG_ID_UNKNOWN, NULL}
};

char* kuaf_alg_get_name(int alg_id);

int kuaf_alg_get_id(char *name);

int kuaf_alg_check(int alg_type, int alg_id);

#endif