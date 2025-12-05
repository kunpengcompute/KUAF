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
#ifndef DEFLATE_ADAPTER_H
#define DEFLATE_ADAPTER_H

#include "zlib.h"
#include "deflate.h"

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

extern int lz_deflateEnd(z_streamp strm);
extern int lz_deflateInit2_(z_streamp strm, int level, int metho, int windowBit, int memLevel, int strategy,
                            const char *version, int stream_size);
extern int lz_deflateReset(z_streamp strm);
extern int lz_deflate(z_streamp strm, int flush);

#  define deflateInit(strm, level) \
          deflateInit_((strm), (level), ZLIB_VERSION, (int)sizeof(z_stream))
#  define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
          deflateInit2_((strm), (level), (method), (windowBits), (memLevel), \
                        (strategy), ZLIB_VERSION, (int)sizeof(z_stream))

ZEXTERN int ZEXPORT compress OF((Bytef *dst,   uLongf *dst_len,
                                 const Bytef *src, uLong src_len));

ZEXTERN int ZEXPORT compress2 OF((Bytef *dst,   uLongf *dst_len,
                                  const Bytef *src, uLong src_len,
                                  int level));

ZEXTERN uLong ZEXPORT compressBound OF((uLong src_len));

ZEXTERN int ZEXPORT deflate OF((z_streamp strm, int flush));

ZEXTERN int ZEXPORT deflateEnd OF((z_streamp strm));

ZEXTERN int ZEXPORT deflateReset OF((z_streamp strm));

ZEXTERN uLong ZEXPORT deflateBound OF((z_streamp strm,
                                       uLong sourceLen));

ZEXTERN int ZEXPORT deflateInit_ OF((z_streamp strm, int level,
                                     const char *version, int stream_size));

ZEXTERN int ZEXPORT deflateInit2_ OF((z_streamp strm, int  level, int  method,
                                      int windowBits, int memLevel,
                                      int strategy, const char *version,
                                      int stream_size));

#endif // DEFLATE_ADAPTER_H