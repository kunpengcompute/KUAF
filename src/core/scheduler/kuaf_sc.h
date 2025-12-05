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
#ifndef KUAF_SC_H
#define KUAF_SC_H

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/**
 * 调度框架状态码
 * 成功码使用0，错误码使用负整数
 */
typedef enum {
    /* 成功状态*/
    KUAF_SUCCESS = 0,            ///<操作成功完成

    KUAF_ERROR              = -1,    // 通用错误
    KUAF_PROCESS_FAIL       = -11,   // 处理失败
    KUAF_NULL_PTR_ERROR     = -12,   // 空指针错误
    KUAF_ALG_NOT_SUPPORT    = -13,   // 不支持的算法
    KUAF_DEV_NOT_SUPPORT    = -14,   // 不支持的设备
    KUAF_STRA_NOT_SUPPORT   = -15,   // 不支持的调度策略
    KUAF_HW_RET_ERROR       = -16,   // 硬件接口返回错误
    KUAF_ALGID_ERROR        = -17,   // 算法类型错误
    KUAF_MALLOC_FAIL        = -18,   // 内存分配失败
} ScheduleStatus;

typedef enum {
    SC_STRATEGY_UNKNOWN,
    SC_BANDWIDTH,
    SC_RATIO,
    SC_PARALLEL
} SC_STRATEGY;

typedef struct {
    void *data;
    int alg_type;
    int alg_id;
    int dev_id;
    SC_STRATEGY strategy;
    float bw_length;
} scheduler_ctx;

#ifdef __cplusplus
extern "C" {
#endif

scheduler_ctx *kuaf_ctx_scheduler_create(int alg_type, int alg_id);

int kuaf_ctx_scheduler(scheduler_ctx *sc_ctx);

int kuaf_ctx_process_sync(scheduler_ctx *sc_ctx);

int kuaf_ctx_end_process(scheduler_ctx *sc_ctx);

void kuaf_ctx_scheduler_free(scheduler_ctx *sc_ctx);

#ifdef __cplusplus
}
#endif

#endif