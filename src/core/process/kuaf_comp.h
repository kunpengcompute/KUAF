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
#ifndef KUAF_COMP_H
#define KUAF_COMP_H

#include "zlib.h"
#include "../algorithm/comp/kuaf_zlib.h"
#include "../scheduler/kuaf_sc.h"

int kuaf_process_comp_zlib_sync(scheduler_ctx *sc_ctx);

extern int lz_deflate(z_streamp strm, int flush);
extern int lz_deflateEnd(z_streamp strm);
extern int lz_deflateInit2_(z_streamp strm, int level, int metho, int windowBit, int memLevel, int strategy,
    const char *version, int stream_size);
extern int lz_deflateReset(z_streamp strm);

extern int lz_inflate(z_streamp strm, int flush);
extern int lz_inflateEnd(z_streamp strm);
extern int lz_inflateInit2_(z_streamp strm, int windowBits, const char *version, int stream_size);
extern int lz_inflateReset(z_streamp strm);

#endif