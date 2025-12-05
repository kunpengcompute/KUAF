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

#ifndef HPRE_RSA_H
#define HPRE_RSA_H

#include <semaphore.h>
#include <asm/types.h>

#include <openssl/rsa.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/bn.h>
#include <openssl/engine.h>


#define RSA_MIN_MODULUS_BITS    512

#define RSA1024BITS     1024
#define RSA2048BITS     2048
#define RSA3072BITS     3072
#define RSA4096BITS     4096

#define HPRE_CONT        (-1)
#define HPRE_CRYPTO_SUCC  1
#define HPRE_CRYPTO_FAIL  0
#define HPRE_CRYPTO_SOFT (-1)

#define OPENSSL_SUCCESS      (1)
#define OPENSSL_FAIL         (0)
#define BLOCKSIZES_OF(data) (sizeof((data)) / sizeof(((data)[0])))
enum {
	INVALID = 0,
	PUB_ENC,
	PUB_DEC,
	PRI_ENC,
	PRI_DEC,
	MAX_CODE,
};

struct bignum_st {
	BN_ULONG *d;
	int top;
	int dmax;
	int neg;
	int flags;
};


RSA_METHOD *kuaf_get_rsa_methods(void);

int kuaf_module_init(void);

void kuaf_destroy(void);

EVP_PKEY_METHOD *kuaf_get_rsa_pkey_meth(void);


int kuaf_pkey_meths(ENGINE *e, EVP_PKEY_METHOD **pmeth, const int **pnids, int nid);

extern hpre_engine_ctx_t *hpre_get_eng_ctx(RSA *rsa, int bits, int type);
extern int wd_hpre_init_qnode_pool(void);
extern int hpre_rsa_soft_calc(int flen, const unsigned char *from, unsigned char *to,
		       RSA *rsa, int padding, int type);

extern int hpre_rsa_soft_genkey(RSA *rsa, int bits, BIGNUM *e, BN_GENCB *cb);
extern int check_rsa_padding(unsigned char *to, int num,
		const unsigned char *buf, int len, int padding, int type);
extern int hpre_get_prienc_res(int padding, BIGNUM *f, const BIGNUM *n, BIGNUM *bn_ret, BIGNUM **res);
extern int hpre_rsa_primegen(int bits, BIGNUM *e_value, BIGNUM *p, BIGNUM *q, BN_GENCB *cb);
extern int hpre_fill_keygen_opdata(void *ctx, struct wcrypto_rsa_op_data *opdata);
extern int hpre_rsa_get_keygen_param(struct wcrypto_rsa_op_data *opdata, void *ctx,
		RSA *rsa, BIGNUM *e_value, BIGNUM *p, BIGNUM *q);
extern int hpre_rsa_sync(void *ctx, struct wcrypto_rsa_op_data *opdata);

extern void hpre_rsa_fill_prikey(RSA *rsa, hpre_engine_ctx_t *eng_ctx, int version,
		const BIGNUM *p, const BIGNUM *q, const BIGNUM *dmp1,
		const BIGNUM *dmq1, const BIGNUM *iqmp);
extern void hpre_free_eng_ctx(hpre_engine_ctx_t *eng_ctx);
extern void hpre_free_bn_ctx_buf(BN_CTX *bn_ctx, unsigned char *in_buf, int num);
extern int hpre_rsa_crypto(hpre_engine_ctx_t *eng_ctx, struct wcrypto_rsa_op_data *opdata);
extern void hpre_rsa_fill_pubkey(const BIGNUM *e, const BIGNUM *n, hpre_engine_ctx_t *rsa_ctx);
extern int hpre_rsa_check_para(int flen, const unsigned char *from,
		unsigned char *to, RSA *rsa);
extern int check_bit_useful(const int bit);
extern int check_pubkey_param(const BIGNUM *n, const BIGNUM *e);
extern EVP_PKEY_METHOD *get_dh_pkey_meth(void);
extern int hpre_rsa_padding(int flen, const unsigned char *from, unsigned char *buf,
		int num, int padding, int type);
#endif

