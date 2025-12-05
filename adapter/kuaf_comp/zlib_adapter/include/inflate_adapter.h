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
#ifndef INFLATE_ADAPTER_H
#define INFLATE_ADAPTER_H

#include "zlib.h"
#include "zutil.h"
#include "inftrees.h"
#include "inflate.h"

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

extern int lz_inflateEnd(z_streamp strm);
extern int lz_inflateInit2_(z_streamp strm, int windowBits, const char *version, int stream_size);
extern int lz_inflateReset(z_streamp strm);
extern int lz_inflate(z_streamp strm, int flush);

#  define inflateInit(strm) \
          inflateInit_((strm), ZLIB_VERSION, (int)sizeof(z_stream))

#  define inflateInit2(strm, windowBits) \
          inflateInit2_((strm), (windowBits), ZLIB_VERSION, \
                        (int)sizeof(z_stream))

ZEXTERN int ZEXPORT uncompress OF((Bytef *dst,   uLongf *dst_len,
                                   const Bytef *src, uLong src_len));

ZEXTERN int ZEXPORT uncompress2 OF((Bytef *dst,   uLongf *dst_len,
                                    const Bytef *src, uLong *src_len));

ZEXTERN int ZEXPORT inflate OF((z_streamp strm, int flush));

ZEXTERN int ZEXPORT inflateEnd OF((z_streamp strm));

ZEXTERN int ZEXPORT inflateReset OF((z_streamp strm));

ZEXTERN int ZEXPORT inflateInit_ OF((z_streamp strm,
                                     const char *version, int stream_size));

ZEXTERN int ZEXPORT inflateInit2_ OF((z_streamp strm, int  windowBits,
                                      const char *version, int stream_size));

#endif