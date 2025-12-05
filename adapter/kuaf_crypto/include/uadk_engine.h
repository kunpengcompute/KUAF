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
#ifndef UADK_H
#define UADK_H
#include <uadk/v1/wd.h>
#include <uadk/v1/wd_rsa.h>
#include <uadk/v1/wd_bmm.h>
#include <openssl/engine.h>
#include <semaphore.h>
#include <pthread.h>  

#define HAVE_WD_MM_BR_DEFINED
#define ARRAY_SIZE(x)		(sizeof(x) / sizeof((x)[0]))
#define ENV_STRING_LEN		256
#define ENGINE_SEND_MAX_CNT	90000000
#define ENGINE_RECV_MAX_CNT	60000000
#define ENGINE_ENV_RECV_MAX_CNT	60000
#define UADK_UNINIT		0
#define UADK_INIT_SUCCESS	1
#define UADK_INIT_FAIL		2
#define UADK_DEVICE_ERROR	3
#define UNSET            0
#define ISSET            1
#define BIT_BYTES_SHIFT  3
#define BN_ULONG        unsigned long
#define MAX_SEND_TRY_CNTS  50
#define MAX_RECV_TRY_CNTS  3000
#define RSA_BALANCE_TIMES 1280
#define WD_STATUS_BUSY      (-EBUSY)

struct hpre_priv_ctx {
	RSA *ssl_alg;
	int is_pubkey_ready;
	int is_privkey_ready;
	int key_size;
};

typedef struct hpre_priv_ctx hpre_priv_ctx_t;

typedef struct KAE_QUEUE_DATA_NODE {
	struct wd_queue            *kae_wd_queue;
	struct wd_queue_mempool    *kae_queue_mem_pool;
	void                       *engine_ctx;
} KAE_QUEUE_DATA_NODE_S;

struct wd_queue_mempool {
	struct wd_queue *q;
	void *base;
	unsigned int *bitmap;
	unsigned int block_size;
	unsigned int block_num;
	unsigned int mem_size;
	unsigned int block_align_size;
	unsigned int free_num;
	unsigned int fail_times;
	unsigned long long index;
	sem_t mempool_sem;
	int dev;
};

struct hpre_engine_ctx {
	void *ctx;
	struct wcrypto_rsa_op_data opdata;
	struct wcrypto_rsa_ctx_setup rsa_setup;
	struct KAE_QUEUE_DATA_NODE *qlist;
	hpre_priv_ctx_t priv_ctx;
};

typedef struct hpre_engine_ctx hpre_engine_ctx_t;

enum {
	HW_V2,
	HW_V3,
};
struct kae_spinlock {
	int lock;
};
typedef struct KAE_QUEUE_POOL_NODE {
	struct kae_spinlock spinlock;
	time_t add_time;
	KAE_QUEUE_DATA_NODE_S *node_data;
} KAE_QUEUE_POOL_NODE_S;

typedef struct KAE_QUEUE_POOL_HEAD {
	int pool_use_num;
	int algtype;  /* alg type,just init at init pool */
	pthread_mutex_t destroy_mutex;
	pthread_mutex_t kae_queue_mutex;
	struct KAE_QUEUE_POOL_HEAD *next;  /* next pool */
	KAE_QUEUE_POOL_NODE_S *kae_queue_pool; /* point to a attray */
} KAE_QUEUE_POOL_HEAD_S;

typedef void (*release_engine_ctx_cb)(void *engine_ctx);
extern const char *engine_uadk_id;
int uadk_e_bind_ciphers(ENGINE *e);
void uadk_e_destroy_ciphers(void);
int uadk_e_bind_digest(ENGINE *e);
void uadk_e_destroy_digest(void);
int uadk_e_bind_rsa(ENGINE *e);
void uadk_e_destroy_rsa(void);
int uadk_e_bind_dh(ENGINE *e);
void uadk_e_destroy_dh(void);
int uadk_e_bind_ecc(ENGINE *e);
void uadk_e_destroy_ecc(void);
int uadk_e_set_env(const char *var_name, int numa_id);
void uadk_e_ecc_lock_init(void);
void uadk_e_rsa_lock_init(void);
void uadk_e_dh_lock_init(void);
void uadk_e_cipher_lock_init(void);
void uadk_e_aead_lock_init(void);
void uadk_e_digest_lock_init(void);
extern int sec_engine_digests(ENGINE *e, const EVP_MD **digest, const int **nids, int nid);
extern int sec_engine_soft_digests(ENGINE *e, const EVP_MD **digest, const int **nids, int nid);
extern void kae_queue_pool_destroy(KAE_QUEUE_POOL_HEAD_S *pool_head, release_engine_ctx_cb release_fn);
extern KAE_QUEUE_POOL_HEAD_S *kae_init_queue_pool(int algtype);
extern KAE_QUEUE_DATA_NODE_S *kae_get_node_from_pool(KAE_QUEUE_POOL_HEAD_S *pool_head);
extern int kae_put_node_to_pool(KAE_QUEUE_POOL_HEAD_S *pool_head, KAE_QUEUE_DATA_NODE_S *node_data);
extern struct uacce_dev *wd_get_accel_dev(const char *alg_name);
#endif
