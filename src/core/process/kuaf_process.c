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
#include "../scheduler/kuaf_sc.h"
#include "../device/kuaf_dev.h"
#include "../algorithm/kuaf_alg.h"
#include "../../common/kuaf_log.h"
#include "../strategy/bandwidth/kuaf_bw.h"
#include "kuaf_comp.h"
#include "kuaf_crypto.h"
#include "kuaf_process.h"

int kuaf_ctx_process_comp_sync(scheduler_ctx *sc_ctx)
{
    int ret = Z_ERRNO;
    switch (sc_ctx->alg_id) {
        case ALG_ZLIB_DEFLATE:
        case ALG_ZLIB_INFLATE:
            ret = kuaf_process_comp_zlib_sync(sc_ctx);
            break;
        default:
            KUAF_ERROR("invalid alg id: %d", sc_ctx->alg_id);
            break;
    }
    return ret;
}

int kuaf_ctx_before_process(scheduler_ctx *sc_ctx)
{
    if (sc_ctx->dev_id == DEV_ID_UNKNOWN) {
        return KUAF_SUCCESS;
    }

    switch (sc_ctx->strategy) {
        case SC_BANDWIDTH:
            (void)kuaf_bw_record(sc_ctx->dev_id, sc_ctx->bw_length);
            break;
        default:
            break;
    }
}

int kuaf_ctx_after_process(scheduler_ctx *sc_ctx)
{
    return KUAF_SUCCESS;
}

int kuaf_ctx_process_crypto_sync(scheduler_ctx *sc_ctx)
{
    int ret = KUAF_ERROR;
    switch (sc_ctx->alg_id) {
        case ALG_RSA:
            ret = kuaf_process_crypto_rsa_sync(sc_ctx);
            break;
        case ALG_SM2:

        default:
            KUAF_ERROR("invalid alg id: %d", sc_ctx->alg_id);
            break;
    }
    return ret;
}