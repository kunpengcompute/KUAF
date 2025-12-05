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
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "../../common/kuaf_config.h"
#include "../../common/kuaf_log.h"
#include "../device/kuaf_dev.h"
#include "../algorithm/kuaf_alg.h"
#include "../strategy/kuaf_stra.h"
#include "../process/kuaf_comp.h"
#include "../process/kuaf_crypto.h"
#include "../process/kuaf_process.h"

#include "securec.h"
#include "kuaf_sc.h"

void kuaf_inner_ctx_free();
void kuaf_init_after_fork();

static int g_paral_initialized = 0;
static int g_bandwidth_initialized = 0;

__attribute__((destructor)) void kuaf_inner_ctx_free()
{
    KUAF_DEBUG("kuaf ctx free.");
    kuaf_bw_free_async();
    usleep(100000);
    kuaf_paral_free();
    kuaf_log_free();
    kuaf_config_free();
}

void kuaf_init_after_fork()
{
    if (kuaf_bw_init_async()) {
        g_bandwidth_initialized = 1;
    } else {
        KUAF_ERROR("kuaf bw init error.");
    }
    kuaf_ratio_init_random_pool();

    pthread_atfork(NULL, NULL, kuaf_init_after_fork);
}

int __attribute__((constructor)) kuaf_inner_ctx_init()
{
    kuaf_config_init();
    kuaf_log_init();
    kuaf_dev_init();

    if (kuaf_paral_init()) {
        g_paral_initialized = 1;
    } else {
        KUAF_ERROR("kuaf paral init error.");
    }

    kuaf_init_after_fork();

    KUAF_DEBUG("kuaf ctx init success.");
    return 1;
}

static int kuaf_scheduler_bandwidth(scheduler_ctx *sc_ctx, char *file_name, char *alg_name)
{
    if (unlikely(!g_bandwidth_initialized)) {
        sc_ctx->dev_id = DEV_ID_UNKNOWN;
        sc_ctx->strategy = SC_BANDWIDTH;
        return KUAF_SUCCESS;
    }
    int dev_id = kuaf_dev_get_id(sc_ctx->alg_id);
    float bw = kuaf_bw_get(dev_id);
    sc_ctx->strategy = SC_BANDWIDTH;
    if (bw < kuaf_config_get_int(file_name, alg_name, KUAF_CONFIG_SCHEDULER_BANDWIDTH)) {
        sc_ctx->dev_id = dev_id;
        KUAF_DEBUG("sc hard. dev_id = %d , bw = %.2f", dev_id, bw);
        return KUAF_SUCCESS;
    }

    KUAF_DEBUG("sc soft. bw = %.2f", bw);
    sc_ctx->dev_id = DEV_ID_UNKNOWN;
    return KUAF_SUCCESS;
}

static int kuaf_scheduler_ratio(scheduler_ctx *sc_ctx, char *file_name, char *alg_name)
{
    int random_val = kuaf_ratio_get_random_value();
    int ratio = kuaf_config_get_int(file_name, alg_name, KUAF_CONFIG_SCHEDULER_RATIO);
    sc_ctx->strategy = SC_RATIO;
    if (random_val <= ratio) {
        sc_ctx->dev_id = kuaf_dev_get_id(sc_ctx->alg_id);
        KUAF_DEBUG("sc hard. dev_id = %d , ratio = %d", sc_ctx->dev_id, ratio);
        return KUAF_SUCCESS;
    }

    KUAF_DEBUG("sc soft. ratio = %d", ratio);
    sc_ctx->dev_id = DEV_ID_UNKNOWN;
    return KUAF_SUCCESS;
}
int kuaf_ctx_end_process(scheduler_ctx *sc_ctx)
{
    if (unlikely(!sc_ctx)) {
        KUAF_ERROR("scheduler_ctx is NULL.");
        return KUAF_NULL_PTR_ERROR;
    }
    if (sc_ctx->dev_id == DEV_ID_UNKNOWN) {
        return KUAF_SUCCESS;
    }
    return KUAF_SUCCESS;
}
static int kuaf_scheduler_parallel(scheduler_ctx *sc_ctx, char *file_name, char *alg_name)
{
    if (unlikely(!g_paral_initialized)) {
        sc_ctx->dev_id = DEV_ID_UNKNOWN;
        sc_ctx->strategy = SC_PARALLEL;
        return KUAF_SUCCESS;
    }
    int dev_id = kuaf_dev_get_id(sc_ctx->alg_id);

    sc_ctx->strategy = SC_PARALLEL;
    if (kuaf_paral_try_P(sc_ctx->alg_id, dev_id) == 1) {
        sc_ctx->dev_id = dev_id;
        return KUAF_SUCCESS;
    }

    sc_ctx->dev_id = DEV_ID_UNKNOWN;
    return KUAF_SUCCESS;
}

int kuaf_ctx_scheduler(scheduler_ctx *sc_ctx)
{
    if (unlikely(!sc_ctx)) {
        KUAF_ERROR("scheduler_ctx is NULL.");
        return KUAF_NULL_PTR_ERROR;
    }
    char *strategy = NULL;
    char *alg_name = kuaf_alg_get_name(sc_ctx->alg_id);
    if (unlikely(!alg_name)) {
        KUAF_ERROR("invalid alg id: %d", sc_ctx->alg_id);
        return KUAF_ALGID_ERROR;
    }
 
    char *config_file_name;
    sc_ctx->dev_id = DEV_ID_UNKNOWN;

    switch (sc_ctx->alg_type) {
        case ALG_COMP:
            strategy = kuaf_config_get_str(KUAF_CONFIG_KAE_FILE_NAME, alg_name, KUAF_CONFIG_SCHEDULER);
            config_file_name = KUAF_CONFIG_KAE_FILE_NAME;
            break;
        case ALG_CRYPTO:
            strategy = kuaf_config_get_str(KUAF_CONFIG_KAE_FILE_NAME, alg_name, KUAF_CONFIG_SCHEDULER);
            config_file_name = KUAF_CONFIG_KAE_FILE_NAME;
            break;
        default:
            KUAF_ERROR("kuaf ctx scheduler failed. invalid alg type: %d", sc_ctx->alg_type);
            return KUAF_ALG_NOT_SUPPORT;
    }

    if (unlikely(strategy == NULL)) {
        return KUAF_STRA_NOT_SUPPORT;
    }

    int ret = KUAF_SUCCESS;
    if (strcmp(strategy, KUAF_CONFIG_SCHEDULER_BANDWIDTH) == 0) {
        ret = kuaf_scheduler_bandwidth(sc_ctx, config_file_name, alg_name);
    } else if (strcmp(strategy, KUAF_CONFIG_SCHEDULER_RATIO) == 0) {
        ret = kuaf_scheduler_ratio(sc_ctx, config_file_name, alg_name);
    } else if (strcmp(strategy, KUAF_CONFIG_SCHEDULER_PARALLEL) == 0) {
        ret = kuaf_scheduler_parallel(sc_ctx, config_file_name, alg_name);
    } else {
        KUAF_ERROR("kuaf ctx scheduler failed. invalid scheduler strategy: %s", strategy);
        return KUAF_STRA_NOT_SUPPORT;
    }

    KUAF_DEBUG("kuaf ctx scheduler sucess. dev_id = %d", sc_ctx->dev_id);
    return ret;
}

int kuaf_ctx_process_sync(scheduler_ctx *sc_ctx)
{
    if (unlikely(!sc_ctx)) {
        KUAF_ERROR("scheduler_ctx is NULL.");
        return KUAF_NULL_PTR_ERROR;
    }
    int ret = kuaf_dev_check_id(sc_ctx->dev_id);
    if (ret == KUAF_DEV_NOT_SUPPORT) {
        return ret;
    }
    switch (sc_ctx->alg_type) {
        case ALG_COMP:
            ret = kuaf_ctx_process_comp_sync(sc_ctx);
            break;
        case ALG_CRYPTO:
            ret = kuaf_ctx_process_crypto_sync(sc_ctx);
            break;
        default:
            KUAF_ERROR("kuaf ctx process failed. invalid alg type: %d", sc_ctx->alg_type);
            ret = KUAF_ALG_NOT_SUPPORT;
    }
    return ret;
}

scheduler_ctx *kuaf_ctx_scheduler_create(int alg_type, int alg_id)
{
    /* 鍒ゆ柇 alg_type 鍙?alg_id 鏄惁鍚堟硶 */
    errno_t ret = kuaf_alg_check(alg_type, alg_id);
    if (ret == 0) {
        KUAF_ERROR("invalid parameter: alg type %d, alg id %d", alg_type, alg_id);
        return NULL;
    }

    scheduler_ctx *ctx = (scheduler_ctx*)malloc(sizeof(scheduler_ctx));
    if (unlikely(!ctx)) {
        KUAF_ERROR("scheduler_ctx malloc failed.");
        return NULL;
    }

    ret = memset_s(ctx, sizeof(scheduler_ctx), 0, sizeof(scheduler_ctx));  // 娓呴浂鎵€鏈夊熀纭€绫诲瀷鎴愬憳
    if (unlikely(ret != EOK)) {
        kuaf_ctx_scheduler_free(ctx);
        KUAF_ERROR("memset_s error occurred.");
        return NULL;
    }
    ctx->alg_type = alg_type;
    ctx->alg_id = alg_id;

    return ctx;
}

void kuaf_ctx_scheduler_free(scheduler_ctx *sc_ctx)
{
    if (sc_ctx == NULL) {
        return;
    }

    // 鏍规嵁绛栫暐鎵ц娓呯悊閫昏緫
    switch (sc_ctx->strategy) {
        case SC_PARALLEL:
            if (sc_ctx->dev_id != DEV_ID_UNKNOWN && sc_ctx->alg_id != ALG_ID_UNKNOWN) {
                kuaf_paral_V(sc_ctx->alg_id, sc_ctx->dev_id);  // 閲婃斁骞惰璧勬簮
            }
            break;
        default:
            break;
    }

    // 閲婃斁 sc_ctx 鍐呴儴鐨勫姩鎬佸垎閰嶈祫婧愶紙濡傛灉鏈夛級
    if (sc_ctx->data) {
        free(sc_ctx->data);  // 鍋囪 data 鏄姩鎬佸垎閰嶇殑
        sc_ctx->data = NULL;
    }

    // 鏈€鍚庨噴鏀?sc_ctx 鏈韩
    free(sc_ctx);
}