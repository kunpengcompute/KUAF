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
#include <stdio.h>
#include <string.h>
#include <dlfcn.h>
#include <openssl/engine.h>

#include "../include/uadk_engine.h"
#include "../include/uadk_async.h"
#include "../include/kae_engine.h"
#include "../include/rsa_hard.h"
#include "../include/sm2_hard.h"
#include "../../../src/common/kuaf_log.h"
#include <kuaf_alg.h>
#include <kuaf_sc.h>
#include <pthread.h>
#include <uadk/wd.h>

#define UADK_CMD_ENABLE_RSA_ENV ENGINE_CMD_BASE
#define UADK_CMD_ENABLE_ECC_ENV (ENGINE_CMD_BASE + 1)
#define KAE_CMD_ENABLE_ASYNC   (ENGINE_CMD_BASE + 6)
#define KAE_CMD_ENABLE_SM3   (ENGINE_CMD_BASE + 7)
#define KAE_CMD_ENABLE_SM4   (ENGINE_CMD_BASE + 8)
#define ENV_STRING_LEN 256
#define UNIT_BYTE_TO_MB 1048576.0

static int uadk_cipher;
static int uadk_digest;
static int uadk_rsa;
static int uadk_dh;
static int uadk_ecc;
static int uadk_inited;
static pthread_mutex_t uadk_engine_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Constants used when creating the ENGINE */
const char *eng_id = "kae";
static const char *engine_kuaf_name = "kuaf hardware engine support";

static int kuaf_rsa;
static int kuaf_ecc;
static int kuaf_inited;
static pthread_mutex_t kuaf_engine_mutex = PTHREAD_MUTEX_INITIALIZER;

static int kuaf_rsa_nosva;
static int kuaf_sm2_nosva;
static int uadk_digest_nosva;
static pthread_mutex_t ctx_mutex = PTHREAD_MUTEX_INITIALIZER;
static scheduler_ctx *rsa_ctx = NULL;
static kuaf_rsa_ctx *rsa_alg_ctx = NULL;
static scheduler_ctx *sm2_ctx = NULL;

extern int sec_ciphers_set_enabled(int, int);
extern int sec_digests_set_enabled(int, int);
extern void sec_ciphers_free_ciphers(void);
extern void sec_digests_free_methods(void);
extern int wd_get_nosva_dev_num(const char *);
extern int async_module_init_v1(void);
extern void engine_init_child_at_fork_handler_v1(void);
extern void kae_debug_close_log(void);
extern int set_sm2_pkey_soft(void);

static const ENGINE_CMD_DEFN g_kuaf_cmd_defns[] = {
    {UADK_CMD_ENABLE_RSA_ENV, "UADK_CMD_ENABLE_RSA_ENV", "Enable or Disable rsa engine environment variable.",
     ENGINE_CMD_FLAG_NUMERIC},
    {
        KAE_CMD_ENABLE_ASYNC,
        "KAE_CMD_ENABLE_ASYNC",
        "Enable or Disable the engine async interface.",
        ENGINE_CMD_FLAG_NUMERIC
    },
    {
        KAE_CMD_ENABLE_SM3,
        "KAE_CMD_ENABLE_SM3",
        "Enable or Disable the SM3.",
        ENGINE_CMD_FLAG_NUMERIC
    },
    {
        KAE_CMD_ENABLE_SM4,
        "KAE_CMD_ENABLE_SM4",
        "Enable or Disable the SM4.",
        ENGINE_CMD_FLAG_NUMERIC
    },
    {UADK_CMD_ENABLE_ECC_ENV, "UADK_CMD_ENABLE_ECC_ENV", "Enable or Disable ecc engine environment variable.",
     ENGINE_CMD_FLAG_NUMERIC},
    {0, NULL, NULL, 0}};

static void __attribute__((constructor)) kuaf_constructor(void) {}

static void __attribute__((destructor)) kuaf_destructor(void) {}

struct kuaf_alg_env_enabled {
    const char *alg_name;
    __u8 env_enabled;
};

static void engine_init_child_at_fork_handler(void)
{
    int ret;

    ret = async_module_init();
    if (!ret)
        fprintf(stderr, "failed to init child async module!\n");
}

static struct kuaf_alg_env_enabled kuaf_env_enabled[] = {{"rsa", 0}, {"ecc", 0}};

typedef enum kae_alg_type_enum { KAE_SM2_TYPE = 0, KAE_RSA_TYPE, KAE_DIGEST_TYPE, KAE_ALG_TYPE_MAX } kae_alg_type_e;

static const char *kae_alg_type_names[] = {
    [KAE_SM2_TYPE] = "sm2", [KAE_RSA_TYPE] = "rsa", [KAE_DIGEST_TYPE] = "digest(md5/sm3)"};

const char *get_alg_string(kae_alg_type_e type)
{
    if (type >= KAE_ALG_TYPE_MAX) {
        return "unknown kae alg type";
    }
    return kae_alg_type_names[type];
}

static void uadk_e_set_env_enabled(const char *alg_name, __u8 value)
{
    KUAF_DEBUG("uadk_e_set_env_enabled start");
    int len = ARRAY_SIZE(kuaf_env_enabled);
    int i = 0;

    while (i < len) {
        if (!strcmp(kuaf_env_enabled[i].alg_name, alg_name)) {
            kuaf_env_enabled[i].env_enabled = value;
            KUAF_DEBUG("set %s env %s", alg_name, value == 0 ? "Disable" : "Enable");
            return;
        }
        i++;
    }
    KUAF_DEBUG("%s is incorrect,Cannot set env enable or not", alg_name);
}

static int uadk_engine_ctrl(ENGINE *e, int cmd, long i, void *p, void (*f)(void))
{
    (void)p;
    (void)f;

    if (!e) {
        fprintf(stderr, "Null Engine\n");
        return 0;
    }

    switch (cmd) {
        case UADK_CMD_ENABLE_RSA_ENV:
            KUAF_DEBUG("%s rsa\n", i == 0 ? "Disable" : "Enable");
            uadk_e_set_env_enabled("rsa", i);
            break;
        case UADK_CMD_ENABLE_ECC_ENV:
            KUAF_DEBUG("%s ecc\n", i == 0 ? "Disable" : "Enable");
            uadk_e_set_env_enabled("ecc", i);
            break;
        case KAE_CMD_ENABLE_ASYNC:
            KUAF_DEBUG("%s async polling\n", i == 0 ? "Disable" : "Enable");
            if (i == 0) {
                kae_disable_async();
            } else {
                kae_enable_async();
            }
            break;
        case KAE_CMD_ENABLE_SM3:
        case KAE_CMD_ENABLE_SM4:
            break;
        default:
            KUAF_WARN("CTRL command not implemented\n");
            return 0;
    }

    return 1;
}

static int eng_destroy(ENGINE *e)
{
    kae_checking_q_sync_destroy();

    if (kuaf_rsa_nosva)
        kuaf_destroy();

    kae_debug_close_log();
    if (kuaf_rsa)
        uadk_e_destroy_rsa();
    if (kuaf_ecc)
        uadk_e_destroy_ecc();

    async_poll_task_free();
    pthread_mutex_lock(&kuaf_engine_mutex);
    kuaf_inited = 0;
    pthread_mutex_unlock(&kuaf_engine_mutex);
    return 1;
}

static int eng_init(ENGINE *e)
{
#ifdef KAE_GMSSL
    return 1;
#else
    int ret;

    pthread_mutex_lock(&kuaf_engine_mutex);
    if (kuaf_inited) {
        pthread_mutex_unlock(&kuaf_engine_mutex);
        return 1;
    }

    kuaf_inited = 1;
    pthread_mutex_unlock(&kuaf_engine_mutex);

    return 1;
#endif
}

static int eng_finish(ENGINE *e)
{
    pthread_mutex_lock(&ctx_mutex);

    if (sm2_ctx) {
        kuaf_ctx_scheduler_free(sm2_ctx);
        sm2_ctx = NULL;
    }

    if (rsa_ctx) {
        kuaf_ctx_scheduler_free(rsa_ctx);
        rsa_ctx = NULL;
    }

    if (rsa_alg_ctx) {
        free(rsa_alg_ctx);
        rsa_alg_ctx = NULL;
    }

    pthread_mutex_unlock(&ctx_mutex);
    return 1;
}

static void bind_soft_alg(ENGINE *e, kae_alg_type_e kae_alg_type, int dev_num)
{
    if (dev_num == 0) {
        KUAF_DEBUG("%s queue run out, switch to soft", get_alg_string(kae_alg_type));
    } else {
        KUAF_DEBUG("%s wd_get_nosva_dev_num faild, no availiable dev, switch to soft", get_alg_string(kae_alg_type));
    }

    switch (kae_alg_type) {
        case KAE_SM2_TYPE:
            set_sm2_pkey_soft();
            ENGINE_set_pkey_meths(e, kuaf_pkey_meths);
            break;
        case KAE_RSA_TYPE:
            ENGINE_set_RSA(e, RSA_get_default_method());
            break;
        case KAE_DIGEST_TYPE:
            ENGINE_set_digests(e, sec_engine_soft_digests);
            break;
        default:
            break;
    }
}

static int bind_fn_kae_alg(ENGINE *e)
{
    printf("start bind_fn_kae_alg (bind v1 algs)\n");
    int dev_num;
    int ret;

    pthread_mutex_lock(&ctx_mutex);

    // SM2 绑定
    sm2_ctx = kuaf_ctx_scheduler_create(ALG_CRYPTO, ALG_SM2);
    if (unlikely(!sm2_ctx)) {
        ;
        ret = KUAF_MALLOC_FAIL;
    } else {
        ret = kuaf_ctx_scheduler(sm2_ctx);
    }

    if (ret == KUAF_SUCCESS && sm2_ctx->dev_id != 0) {
        if (!hpre_module_sm2_init()) {
            fprintf(stderr, "uadk bind sm2 failed\n");
        } else {
            kuaf_sm2_nosva = 1;
            KUAF_DEBUG("ENGINE_set_pkey_meths sm2 successed (bind v1 sm2)");
        }
    } else {
        kuaf_ctx_scheduler_free(sm2_ctx);
        sm2_ctx = NULL;
        pthread_mutex_unlock(&ctx_mutex);
        bind_soft_alg(e, KAE_SM2_TYPE, 0);
    }

    // RSA 绑定
    rsa_ctx = kuaf_ctx_scheduler_create(ALG_CRYPTO, ALG_RSA);
    rsa_alg_ctx = (kuaf_rsa_ctx *)calloc(1, sizeof(kuaf_rsa_ctx));
    if (unlikely(!rsa_ctx) || unlikely(!rsa_alg_ctx)) {
        ret = KUAF_MALLOC_FAIL;
    } else {
        ret = kuaf_ctx_scheduler(rsa_ctx);
    }

    pthread_mutex_unlock(&ctx_mutex);

    dev_num = wd_get_nosva_dev_num("rsa");
    if (rsa_ctx->dev_id != 0 && ret == KUAF_SUCCESS) {
        rsa_alg_ctx->sche_ctx = rsa_ctx;
        kuaf_module_init();
        ENGINE_set_pkey_meths(e, kuaf_pkey_meths);
        if (!ENGINE_set_RSA(e, kuaf_get_rsa_methods())) {
            fprintf(stderr, "uadk bind rsa failed\n");
        } else {
            kuaf_rsa_nosva = 1;
            KUAF_DEBUG("ENGINE_set_RSA successed (bind v1 rsa)");
        }
    } else {
        kuaf_ctx_scheduler_free(rsa_ctx);
        rsa_ctx = NULL;
        pthread_mutex_unlock(&ctx_mutex);

        bind_soft_alg(e, KAE_RSA_TYPE, dev_num);
    }

    if (dev_num > 0) {
        digest_module_init();
        if (!ENGINE_set_digests(e, sec_engine_digests)) {
            fprintf(stderr, "uadk bind digest failed\n");
        } else {
            uadk_digest_nosva = 1;
        }
    } else {
        bind_soft_alg(e, KAE_DIGEST_TYPE, dev_num);
    }
    return KUAF_SUCCESS;
}

int get_dev_by_sync_mode(int inlen)
{
    pthread_mutex_lock(&ctx_mutex);

    if (!rsa_alg_ctx || !rsa_alg_ctx->sche_ctx) {
        pthread_mutex_unlock(&ctx_mutex);
        fprintf(stderr, "Error: rsa_ctx or sche_ctx is NULL\n");
        return -1;
    }

    rsa_alg_ctx->from_len = inlen;
    scheduler_ctx *ctx = rsa_alg_ctx->sche_ctx;
    ctx->bw_length = (float)inlen / UNIT_BYTE_TO_MB;

    int ret = kuaf_ctx_process_sync(ctx);

    pthread_mutex_unlock(&ctx_mutex);
    return ret;
}

static void bind_fn_uadk_alg(ENGINE *e)
{
    struct uacce_dev *dev;

    dev = wd_get_accel_dev("cipher");
    if (dev) {
        if (!uadk_e_bind_ciphers(e)) {
            fprintf(stderr, "uadk bind cipher failed\n");
        } else {
            uadk_cipher = 1;
        }
        free(dev);
    } else {
        KUAF_DEBUG("cipher use wd_get_accel_dev faild ,no availiable dev_num");
    }

    dev = wd_get_accel_dev("digest");
    if (dev) {
        if (!uadk_e_bind_digest(e)) {
            fprintf(stderr, "uadk bind digest failed\n");
        } else {
            uadk_digest = 1;
            KUAF_DEBUG("uadk_e_bind_digest successed (bind v2 digest)");
        }
        free(dev);
    } else {
        KUAF_DEBUG("digest use wd_get_accel_dev faild ,no availiable dev_num");
    }

    dev = wd_get_accel_dev("rsa");
    if (dev) {
        if (!uadk_e_bind_rsa(e)) {
            fprintf(stderr, "uadk bind rsa failed\n");
        } else {
            uadk_rsa = 1;
            KUAF_DEBUG("uadk_e_bind_rsa successed (bind v2 rsa)");
        }
        free(dev);
    } else {
        KUAF_DEBUG("rsa use wd_get_accel_dev faild ,no availiable dev_num");
    }

    dev = wd_get_accel_dev("dh");
    if (dev) {
        if (!uadk_e_bind_dh(e)) {
            fprintf(stderr, "uadk bind dh failed\n");
        } else {
            uadk_dh = 1;
            KUAF_DEBUG("uadk_e_bind_dh successed (bind v2 dh)");
        }
        free(dev);
    } else {
        KUAF_DEBUG("dh use wd_get_accel_dev faild ,no availiable dev_num");
    }

    /* find an ecc device, no difference for sm2/ecdsa/ecdh/x25519/x448 */
    dev = wd_get_accel_dev("ecdsa");
    if (dev) {
        if (!uadk_e_bind_ecc(e)) {
            fprintf(stderr, "uadk bind ecc failed\n");
        } else {
            uadk_ecc = 1;
            KUAF_DEBUG("uadk_e_bind_ecc successed (bind v2 ecc)");
        }
        free(dev);
    } else {
        KUAF_DEBUG("ecdsa use wd_get_accel_dev faild ,no availiable dev_num");
    }
}

/*
 * Connect uadk_engine to OpenSSL engine library.
 */
static int bind_fn(ENGINE *e, const char *id)
{
    int ret;
    if (!ENGINE_set_id(e, eng_id) || !ENGINE_set_destroy_function(e, eng_destroy) ||
        !ENGINE_set_init_function(e, eng_init) || !ENGINE_set_finish_function(e, eng_finish) ||
        !ENGINE_set_name(e, engine_kuaf_name)) {
        fprintf(stderr, "bind failed\n");
        return 0;
    }

    bind_fn_kae_alg(e);
    if (kuaf_rsa_nosva) {
        async_module_init_v1();
        pthread_atfork(NULL, NULL, engine_init_child_at_fork_handler_v1);
        KUAF_INFO("enable nosva");
    }
    bind_fn_uadk_alg(e);

    if (uadk_cipher || uadk_digest || uadk_rsa || uadk_dh || uadk_ecc) {
        pthread_atfork(NULL, NULL, engine_init_child_at_fork_handler);
    }
    ret = ENGINE_set_ctrl_function(e, uadk_engine_ctrl);
    if (ret != 1) {
        fprintf(stderr, "failed to set ctrl function\n");
        return 0;
    }

    ret = ENGINE_set_cmd_defns(e, g_kuaf_cmd_defns);
    if (ret != 1) {
        fprintf(stderr, "failed to set defns\n");
        return 0;
    }

    return 1;
}

IMPLEMENT_DYNAMIC_CHECK_FN()
IMPLEMENT_DYNAMIC_BIND_FN(bind_fn)