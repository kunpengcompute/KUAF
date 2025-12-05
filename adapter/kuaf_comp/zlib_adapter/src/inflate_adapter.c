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
#include "inflate_adapter.h"

#include "kuaf_sc.h"
#include "kuaf_alg.h"

int ZEXPORT inflateInit2_(strm, windowBits, version, stream_size)
z_streamp strm;
int windowBits;
const char *version;
int stream_size;
{
    if (kz_get_devices()) {
        int ret = lz_inflateInit2_(strm, windowBits, version, stream_size);
        if (ret != Z_OK) {
            return Z_ERRNO;
        }
        scheduler_ctx* ctx = kuaf_ctx_scheduler_create(ALG_COMP, ALG_ZLIB_INFLATE);
        if (unlikely(!ctx)) {
            return Z_ERRNO;
        }

        kuaf_zlib_ctx* zlib_ctx = malloc(sizeof(kuaf_zlib_ctx));
        if (unlikely(!zlib_ctx)) {
            kuaf_ctx_scheduler_free(ctx);
            return Z_ERRNO;
        }
        zlib_ctx->strm  = strm;
        zlib_ctx->flush = 0;
        ctx->data = (void*) zlib_ctx;

        /* insert sche_ctx into strm->state */
        struct inflate_state FAR *state;
        state = (struct inflate_state FAR *)strm->state;
        state->sche_ctx = (uLong)ctx;

        ret = kuaf_ctx_scheduler(ctx);
        if (ret != KUAF_SUCCESS) {
            free(ctx->data);
            ctx->data = NULL;
            kuaf_ctx_scheduler_free(ctx);
            state->sche_ctx = 0;
            return Z_ERRNO;
        }

        /* Call KAE Hardware */
        if (ctx->dev_id != 0) {
            return kz_inflateInit2_(strm, windowBits, version, stream_size);
        }
        return Z_OK;
    } else {
        return lz_inflateInit2_(strm, windowBits, version, stream_size);
    }
}

int ZEXPORT inflateInit_(strm, version, stream_size)
z_streamp strm;
const char *version;
int stream_size;
{
    return inflateInit2_(strm, DEF_WBITS, version, stream_size);
}

int ZEXPORT inflateReset(strm)
z_streamp strm;
{
    if (kz_get_devices()) {
        int ret = Z_OK;
        struct inflate_state FAR *state;
        state = (struct inflate_state FAR *)strm->state;
        scheduler_ctx* ctx = (scheduler_ctx*)state->sche_ctx;
        if (ctx->dev_id != 0) {
            ret = kz_inflateReset(strm);
        }
        if (ret != Z_OK) {
            free(ctx->data);
            ctx->data = NULL;
            kuaf_ctx_scheduler_free(ctx);
            state->sche_ctx = 0;
            return ret;
        }
        return lz_inflateReset(strm);
    } else {
        return lz_inflateReset(strm);
    }
}

int ZEXPORT inflate(strm, flush)
z_streamp strm;
int flush;
{
    if (kz_get_devices()) {
        struct inflate_state FAR *state;
        state = (struct inflate_state FAR *)strm->state;
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
        return lz_inflate(strm, flush);
    }
}

int ZEXPORT inflateEnd(strm)
z_streamp strm;
{
    if (kz_get_devices()) {
        int ret = Z_OK;
        struct inflate_state FAR *state;
        state = (struct inflate_state FAR *)strm->state;
        scheduler_ctx* ctx = (scheduler_ctx*)state->sche_ctx;
        if (ctx->dev_id != 0) {
            ret = kz_inflateEnd(strm);
            kuaf_ctx_end_process(ctx);
        }
        if (ctx->data != NULL) {
            free(ctx->data);
            ctx->data = NULL;
        }
        kuaf_ctx_scheduler_free(ctx);
        state->sche_ctx = 0;
        return ret == Z_OK ? lz_inflateEnd(strm) : ret;
    } else {
        return lz_inflateEnd(strm);
    }
}

int ZEXPORT uncompress2 (dst, dst_len, src, src_len)
    Bytef *dst;
    uLongf *dst_len;
    const Bytef *src;
    uLong *src_len;
{
    z_stream strm;
    int error;
    const uInt max = ((uInt)-1) - 3; // make sure its multiples of 4B
    uLong len, left;
    Byte buf[1];
    len = *src_len;
    if (*dst_len) {
        left = *dst_len;
        *dst_len = 0;
    }
    else {
        left = 1;
        dst = buf;
    }

    strm.next_in = (z_const Bytef *)src;
    strm.avail_in = 0;
    strm.zalloc = (alloc_func)0;
    strm.zfree = (free_func)0;
    strm.opaque = (voidpf)0;

    error = inflateInit(&strm);
    if (error != Z_OK) return error;

    strm.next_out = dst;
    strm.avail_out = 0;

    do {
        if (strm.avail_out == 0) {
            strm.avail_out = left > (uLong)max ? max : (uInt)left;
            left -= strm.avail_out;
        }
        if (strm.avail_in == 0) {
            strm.avail_in = len > (uLong)max ? max : (uInt)len;
            len -= strm.avail_in;
        }
        error = inflate(&strm, Z_NO_FLUSH);
    } while (error == Z_OK);

    *src_len -= len + strm.avail_in;
    if (dst != buf)
        *dst_len = strm.total_out;
    else if (strm.total_out && error == Z_BUF_ERROR)
        left = 1;

    inflateEnd(&strm);
    return error == Z_STREAM_END ? Z_OK :
           error == Z_NEED_DICT ? Z_DATA_ERROR  :
           error == Z_BUF_ERROR && left + strm.avail_out ? Z_DATA_ERROR :
           error;
}

int ZEXPORT uncompress (dst, dst_len, src, src_len)
    Bytef *dst;
    uLongf *dst_len;
    const Bytef *src;
    uLong src_len;
{
    return uncompress2(dst, dst_len, src, &src_len);
}
