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

#ifndef HPRE_SM2_H
#define HPRE_SM2_H

#include <uadk/v1/wd_ecc.h>
#include <sched.h>
#include "uadk_engine.h"

extern KAE_QUEUE_POOL_HEAD_S *g_hpre_sm2_qnode_pool;

#define HPRE_SM2_RETURN_FAIL_IF(cond, mesg, ret) \
	do { \
		if (unlikely(cond)) { \
			KUAF_ERROR(mesg); \
				return (ret); \
		} \
	} while (0)

#define HPRE_SM2_DO_SOFT			(-0xE0)
#define TRANS_BITS_BYTES_SHIFT		3
#define ECC_POINT_SIZE(n)		((n) * 2)
#define GET_MS_BYTE(n)			((n) >> 8)
#define GET_LS_BYTE(n)			((n) & 0xFF)
#define DGST_SHIFT_NUM(n)		(8 - ((n) & 0x7))
#define ECC_TYPE	5
#define BITS_TO_BYTES(bits)		(((bits) + 7) >> 3)

#define HPRE_SM2_DO_SOFT			(-0xE0)
#define SM2_MAX_KEY_BYTES		66
#define SM2_GET_RAND_MAX_CNT	100
#define SM2_OCTET_STRING		0x04
#define SM2_ECC_PUBKEY_PARAM_NUM	2
#define SM2_MAX_KEY_BYTES		66
#define SM2_KEY_BYTES			32
#define MAX_SEND_TRY_CNTS  50

typedef int (*PFUNC_SIGN)(EVP_PKEY_CTX *ctx,
			  unsigned char *sig,
			  size_t *siglen,
			  const unsigned char *tbs,
			  size_t tbslen);

typedef int (*PFUNC_VERIFY)(EVP_PKEY_CTX *ctx,
			    const unsigned char *sig,
			    size_t siglen,
			    const unsigned char *tbs,
			    size_t tbslen);
typedef int (*PFUNC_ENC)(EVP_PKEY_CTX *ctx,
			 unsigned char *out,
			 size_t *outlen,
			 const unsigned char *in,
			 size_t inlen);
typedef int (*PFUNC_DEC)(EVP_PKEY_CTX *ctx,
			 unsigned char *out,
			 size_t *outlen,
			 const unsigned char *in,
			 size_t inlen);

enum {
	HPRE_SM2_INIT_FAIL = -1,
	HPRE_SM2_UNINIT,
	HPRE_SM2_INIT_SUCC
};

enum {
	MD_UNCHANGED,
	MD_CHANGED
};

typedef struct hpre_sm2_ciphertext {
	BIGNUM *C1x;
	BIGNUM *C1y;
	ASN1_OCTET_STRING *C3;
	ASN1_OCTET_STRING *C2;
} HPRE_SM2_Ciphertext;

struct hpre_sm2_param {
	/*
	 * p: BIGNUM with the prime number (GFp) or the polynomial
	 * defining the underlying field (GF2m)
	 */
	BIGNUM *p;
	/* a: BIGNUM for parameter a of the equation */
	BIGNUM *a;
	/* b: BIGNUM for parameter b of the equation */
	BIGNUM *b;
	/* xG: BIGNUM for the x-coordinate value of G point */
	BIGNUM *xG;
	/* yG: BIGNUM for the y-coordinate value of G point */
	BIGNUM *yG;
	/* xA: BIGNUM for the x-coordinate value of PA point */
	BIGNUM *xA;
	/* yA: BIGNUM for the y-coordinate value of PA point */
	BIGNUM *yA;
};

typedef struct hpre_sm2_engine_ctx hpre_sm2_engine_ctx_t;

typedef struct {
	/* Key and paramgen group */
	EC_GROUP *gen_group;
	/* Message digest */
	const EVP_MD *md;
	/* Distinguishing Identifier, ISO/IEC 15946-3 */
	uint8_t *id;
	size_t id_len;
	/* Indicates if the 'id' field is set (1) or not (0) */
	int id_set;
} HPRE_SM2_PKEY_CTX;

struct hpre_sm2_priv_ctx {
	HPRE_SM2_PKEY_CTX ctx;
	// handle_t sess;
	const BIGNUM *prikey;
	const EC_POINT *pubkey;
	BIGNUM *order;
	int init_status;
	/* The nid of digest method */
	int md_nid;
	/* The update status of digest method, changed (1), not changed (0) */
	int md_update_status;
	hpre_sm2_engine_ctx_t *e_hpre_sm2_ctx;
};

typedef struct hpre_sm2_priv_ctx hpre_sm2_priv_ctx_t;
struct hpre_sm2_engine_ctx {
	void * wd_ctx;
	struct wcrypto_ecc_op_data opdata;
	struct wcrypto_ecc_ctx_setup setup;
	KAE_QUEUE_DATA_NODE_S *qlist;
	hpre_sm2_priv_ctx_t *priv_ctx;
};

extern int hpre_get_sm2_pkey_meths(ENGINE *e, EVP_PKEY_METHOD **pmeth, const int **nids, int nid);
int hpre_module_sm2_init(void);
extern void wd_sm2_uninit_qnode_pool(void);
extern void hpre_sm2_set_enabled(int nid, int enabled);
extern EVP_PKEY_METHOD *get_sm2_pkey_meth(void);
extern KAE_QUEUE_POOL_HEAD_S *wd_hpre_sm2_get_qnode_pool(void);
extern EVP_PKEY_METHOD *get_sm2_pkey_meth(void);
extern void *kae_wd_alloc_blk(void *pool, size_t size);
extern void kae_wd_free_blk(void *pool, void *blk);
extern void *kae_dma_map(void *usr, void *va, size_t sz);
extern void kae_dma_unmap(void *usr, void *va, void *dma, size_t sz);
#endif

