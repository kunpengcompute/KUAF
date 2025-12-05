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
#include <assert.h>
#include <iostream>
#include "kuaf_sc.h"
#include "kuaf_alg.h"

int main()
{
    scheduler_ctx *ptr = NULL;
    int32_t alg_type = -1024;
    int32_t alg_id = -1024;
    scheduler_ctx *ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    assert(ctx == NULL && "scheduler_ctx not be NULL");

    int ret_sc = kuaf_ctx_scheduler(ptr);
    assert(ret_sc == KUAF_NULL_PTR_ERROR && "ret_sc not be NULL");

    int ret_process = kuaf_ctx_process_sync(ptr);
    assert(ret_process == KUAF_NULL_PTR_ERROR && "ret_process not be NULL");

    int ret_end = kuaf_ctx_end_process(ptr);
    assert(ret_end == KUAF_NULL_PTR_ERROR && "ret_end not be NULL");

    kuaf_ctx_scheduler_free(ctx);
    std::cout<< "kuaf ci fuzz test success." << std::endl;
    return 0;
}