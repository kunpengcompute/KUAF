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
#include "zlib.h"
#include "kaezip.h"
#include "deflate_adapter.h"

#include "kuaf_sc.h"
#include "kuaf_alg.h"

int ZEXPORT deflateInit2_(strm, level, method, windowBits, memLevel, strategy,
                  version, stream_size)
    z_streamp strm;
    int  level;
    int  method;
    int  windowBits;
    int  memLevel;
    int  strategy;
    const char *version;
    int stream_size;
{
    if (kz_get_devices()) {
        int ret = lz_deflateInit2_(strm, level, method, windowBits, memLevel, strategy,
                version, stream_size);
        if (ret != Z_OK) {
            return Z_ERRNO;
        }
        scheduler_ctx* ctx = kuaf_ctx_scheduler_create(ALG_COMP, ALG_ZLIB_DEFLATE);
        if (unlikely(!ctx)) {
            return Z_ERRNO;
        }

        kuaf_zlib_ctx* zlib_ctx = (kuaf_zlib_ctx*)malloc(sizeof(kuaf_zlib_ctx));
        if (unlikely(!zlib_ctx)) {
            kuaf_ctx_scheduler_free(ctx);
            return Z_ERRNO;
        }
        zlib_ctx->strm  = strm;
        zlib_ctx->flush = 0;
        ctx->data = (void*) zlib_ctx;
        strm->state->sche_ctx = (uLong)ctx;

        ret = kuaf_ctx_scheduler(ctx);
        if (ret != KUAF_SUCCESS) {
            free(ctx->data);
            ctx->data = NULL;
            kuaf_ctx_scheduler_free(ctx);
            strm->state->sche_ctx = 0;
            return Z_ERRNO;
        }
        
        /* Call KAE Hardware */
        if (ctx->dev_id != 0) {
            return kz_deflateInit2_(strm, level, method, windowBits, memLevel, strategy,
                    version, stream_size);
        }
        return Z_OK;
    } else {
        return lz_deflateInit2_(strm, level, method, windowBits, memLevel, strategy,
                version, stream_size);
    }
}

int ZEXPORT deflateInit_(strm, level, version, stream_size)
    z_streamp strm;
    int level;
    const char *version;
    int stream_size;
{
    return deflateInit2_(strm, level, Z_DEFLATED, MAX_WBITS, DEF_MEM_LEVEL,
                         Z_DEFAULT_STRATEGY, version, stream_size);
    /* To do: ignore strm->next_in if we use it as window */
}

int ZEXPORT deflate(strm, flush)
z_streamp strm;
int flush;
{
    if (kz_get_devices()) {
        deflate_state *state = (deflate_state *)strm->state;
        scheduler_ctx* ctx = (scheduler_ctx*)state->sche_ctx;
        
        kuaf_zlib_ctx* zlib_ctx = (kuaf_zlib_ctx*)ctx->data;
        zlib_ctx->flush = flush;

        int ret = kuaf_ctx_process_sync(ctx);
        // ret < 0 (except Z_BUF_ERROR) means process failed
        if (ret < 0 && ret != Z_BUF_ERROR) {
            free(ctx->data);
            ctx->data = NULL;
            kuaf_ctx_scheduler_free(ctx);
            state->sche_ctx = 0;
            ret = Z_ERRNO;
        }
        return ret;
    } else {
        return lz_deflate(strm, flush);
    }
}

int ZEXPORT deflateReset(strm)
z_streamp strm;
{
    if (kz_get_devices()) {
        int ret = Z_OK;
        deflate_state *state = (deflate_state *)strm->state;
        scheduler_ctx* ctx = (scheduler_ctx*)state->sche_ctx;
        if (ctx->dev_id != 0) {
            ret = kz_deflateReset(strm); // 硬算任务
        }
        if (ret != Z_OK) {
            free(ctx->data);
            ctx->data = NULL;
            kuaf_ctx_scheduler_free(ctx);
            state->sche_ctx = 0;
            return ret;
        }
        return lz_deflateReset(strm);
    } else {
        return lz_deflateReset(strm);
    }
}

int ZEXPORT deflateEnd (strm)
    z_streamp strm;
{
    if (kz_get_devices()) {
        int ret = Z_OK;
        deflate_state *state = (deflate_state *)strm->state;
        scheduler_ctx* ctx = (scheduler_ctx*)state->sche_ctx;
        if (ctx->dev_id != 0) {
            ret = kz_deflateEnd(strm);
            kuaf_ctx_end_process(ctx);
        }
        if (ctx->data != NULL) {
            free(ctx->data);
            ctx->data = NULL;
        }
        kuaf_ctx_scheduler_free(ctx);
        state->sche_ctx = 0;
        return ret == Z_OK ? lz_deflateEnd(strm) : ret;
    } else {
        return lz_deflateEnd(strm);
    }
}

uLong ZEXPORT deflateBound(strm, sourceLen)
    z_streamp strm;
    uLong sourceLen;
{
    // add 13 is to make sure that the output buffer will be large enough
    return sourceLen + (sourceLen >> 3) + 13;
}


int ZEXPORT compress2 (dst, dst_len, src, src_len, level)
    Bytef *dst;
    uLongf *dst_len;
    const Bytef *src;
    uLong src_len;
    int level;
{
    z_stream strm;
    int error;
    const uInt max = ((uInt)-1) - 3; // make sure its multiples of 4B
    uLong left;

    left = *dst_len;
    *dst_len = 0;

    strm.zalloc = (alloc_func)0;
    strm.zfree = (free_func)0;
    strm.opaque = (voidpf)0;

    error = deflateInit(&strm, level);
    if (error != Z_OK) return error;

    strm.next_out = dst;
    strm.avail_out = 0;
    strm.next_in = (z_const Bytef *)src;
    strm.avail_in = 0;

    do {
        if (strm.avail_out == 0) {
            strm.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= strm.avail_out;
        }
        if (strm.avail_in == 0) {
            strm.avail_in = src_len > (uLong)max ? max : (uInt)src_len;
            src_len -= strm.avail_in;
        }
        error = deflate(&strm, src_len ? Z_NO_FLUSH : Z_FINISH);
    } while (error == Z_OK);

    *dst_len = strm.total_out;
    deflateEnd(&strm);
    return error == Z_STREAM_END ? Z_OK : error;
}

/* ===========================================================================
 */
int ZEXPORT compress (dst, dst_len, src, src_len)
    Bytef *dst;
    uLongf *dst_len;
    const Bytef *src;
    uLong src_len;
{
    return compress2(dst, dst_len, src, src_len, Z_DEFAULT_COMPRESSION);
}

uLong ZEXPORT compressBound (src_len)
    uLong src_len;
{
    // add 13 is to make sure that the output buffer will be large enough
    return src_len + (src_len >> 3) + 13;
}