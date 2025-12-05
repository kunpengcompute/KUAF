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
#include "kuaf_comp.h"
#include "kaezip.h"
#include "../strategy/bandwidth/kuaf_bw.h"
#include "../device/kuaf_dev.h"
#include "../algorithm/kuaf_alg.h"
#include "../../common/kuaf_log.h"
#include "kuaf_process.h"

int process_zlib_deflate_sync(scheduler_ctx *sc_ctx)
{
    kuaf_zlib_ctx *zlib_ctx = (kuaf_zlib_ctx *)sc_ctx->data;
    if (unlikely(!zlib_ctx || !zlib_ctx->strm)) {
        KUAF_ERROR("zlib deflate sync process failed, zlib context or strm is null.");
        return KUAF_NULL_PTR_ERROR;
    }
    z_streamp strm = zlib_ctx->strm;
    int flush = zlib_ctx->flush;

    if (sc_ctx->dev_id == DEV_ID_UNKNOWN) {
        KUAF_INFO("zlib deflate switch to cpu.");
        return lz_deflate(strm, flush);
    }

    sc_ctx->bw_length = strm->avail_in / UNIT_BYTE_TO_MB;
    kuaf_ctx_before_process(sc_ctx);

    int ret = kz_deflate(strm, flush);
    if (ret == Z_CALL_SOFT) {
        KUAF_WARN("zlib deflate kae process failed, switch to cpu.");
        return lz_deflate(strm, flush);
    } else if (ret < 0) {
        KUAF_ERROR("kz_deflate failed, ret = %d.", ret);
        return ret;
    }
    KUAF_DEBUG("zlib deflate kae process success.");
    /* 将硬算接口的返回值向外透传 */
    return ret;
}

int process_zlib_inflate_sync(scheduler_ctx *sc_ctx)
{
    kuaf_zlib_ctx *zlib_ctx = (kuaf_zlib_ctx *)sc_ctx->data;
    if (unlikely(!zlib_ctx || !zlib_ctx->strm)) {
        KUAF_ERROR("zlib inflate sync process failed, zlib context or strm is null.");
        return Z_ERRNO;
    }

    z_streamp strm = zlib_ctx->strm;
    int flush = zlib_ctx->flush;

    if (sc_ctx->dev_id == DEV_ID_UNKNOWN) {
        KUAF_INFO("zlib inflate switch to cpu.");
        return lz_inflate(strm, flush);
    }

    sc_ctx->bw_length = strm->avail_in / UNIT_BYTE_TO_MB;
    kuaf_ctx_before_process(sc_ctx);

    int ret = kz_inflate(strm, flush);
    if (ret == Z_CALL_SOFT) {
        KUAF_WARN("zlib inflate kae process failed, switch to cpu.");
        return lz_inflate(strm, flush);
    } else if (ret < 0) {
        KUAF_ERROR("kz_inflate failed, ret = %d.", ret);
        return ret;
    }
    KUAF_DEBUG("zlib inflate kae process success.");
    return ret;
}

int kuaf_process_comp_zlib_sync(scheduler_ctx *sc_ctx)
{
    int ret = Z_ERRNO;
    switch (sc_ctx->alg_id) {
        case ALG_ZLIB_DEFLATE:
            ret = process_zlib_deflate_sync(sc_ctx);
            break;
        case ALG_ZLIB_INFLATE:
            ret = process_zlib_inflate_sync(sc_ctx);
            break;
        default:
            break;
    }
    return ret;
}