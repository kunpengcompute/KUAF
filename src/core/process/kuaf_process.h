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

#ifndef KUAF_PROCESS_H
#define KUAF_PROCESS_H

int kuaf_ctx_process_comp_sync(scheduler_ctx *sc_ctx);
int kuaf_ctx_process_crypto_sync(scheduler_ctx *sc_ctx);

int kuaf_ctx_before_process(scheduler_ctx *sc_ctx);

int kuaf_ctx_after_process(scheduler_ctx *sc_ctx);

#endif