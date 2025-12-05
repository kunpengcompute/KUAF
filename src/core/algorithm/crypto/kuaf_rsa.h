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
#ifndef KUAF_RSA_H
#define KUAF_RSA_H

#include <openssl/rsa.h>



typedef struct {
    // base data
    void *sche_ctx;
    RSA *rsa;

    // enc_dec data
    const unsigned char *from;
    int from_len;
    unsigned char *to;
    int padding;

    // keygen data
    int bits;
    BIGNUM *e;
    BN_GENCB *cb;
} kuaf_rsa_ctx;

#endif