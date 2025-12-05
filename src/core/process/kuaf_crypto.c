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
#include "kuaf_crypto.h"
#include "../algorithm/crypto/kuaf_rsa.h"
#include "../strategy/bandwidth/kuaf_bw.h"
#include "kuaf_process.h"
int kuaf_process_crypto_rsa_sync(scheduler_ctx *sc_ctx)
{
/*
    if (sc_ctx->dev_id == DEV_ID_UNKNOWN) {
        KUAF_INFO("zlib deflate switch to cpu.");
        return bind_kae_soft_alg(strm, flush);
    }
*/

    kuaf_ctx_before_process(sc_ctx);

    sc_ctx->data = NULL;

    /* 将硬算接口的返回值向外透传 */
    return KUAF_SUCCESS;
}