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
#ifndef UADK_ASYNC_H
#define UADK_ASYNC_H

#include <stdbool.h>
#include <semaphore.h>
#include <openssl/async.h>

#define ASYNC_QUEUE_TASK_NUM	1024
#define UADK_E_SUCCESS		1
#define UADK_E_FAIL		0
#define DO_SYNC			1

#define ASYNC_JOB_RESUMED_UNEXPECTEDLY (-1)
#define ASYNC_CHK_JOB_RESUMED_UNEXPECTEDLY(x) \
	((x) == ASYNC_JOB_RESUMED_UNEXPECTEDLY)
#define ASYNC_STATUS_UNSUPPORTED 0
#define ASYNC_STATKUAF_ERROR         1
#define ASYNC_STATUS_OK          2
#define ASYNC_STATUS_EAGAIN      3

struct async_op {
	ASYNC_JOB *job;
	int done;
	int idx;
	int ret;
};

enum task_type_wd {
	ASYNC_TASK_WD_CIPHER = 0x1,
	ASYNC_TASK_WD_DIGEST,
	ASYNC_TASK_WD_AEAD,
	ASYNC_TASK_WD_RSA,
	ASYNC_TASK_WD_DH,
	ASYNC_TASK_WD_ECC,
	ASYNC_TASK_WD_MAX
};

struct uadk_e_cb_info {
	void *priv;
	struct async_op *op;
};

typedef int (*async_recv_t)(void *ctx);

enum task_type {
	ASYNC_TASK_CIPHER = 0x1,
	ASYNC_TASK_DIGEST,
	ASYNC_TASK_AEAD,
	ASYNC_TASK_RSA,
	ASYNC_TASK_DH,
	ASYNC_TASK_ECC,
	ASYNC_TASK_MAX
};

struct async_queue {
    struct async_task *head;
    int status[ASYNC_QUEUE_TASK_NUM];
    int send_idx;
    int recv_idx;
    bool receiving;
    sem_t empty_sem;
    sem_t full_sem;
    pthread_mutex_t task_mutex;
    pthread_t tid;
    pthread_attr_t thread_attr;
};

enum poll_status {
    POLL_DISABLED,
    POLL_ENABLED
};

struct async_task {
    enum task_type type;
    void *context;
    struct async_operation *operation;
};

typedef struct {
	volatile int flag;
	volatile int verifyRst;
	volatile ASYNC_JOB *job;
} op_done_t;

extern int async_setup_async_event_notification(struct async_op *op);
extern int async_clear_async_event_notification(void);
extern int async_pause_job(void *ctx, struct async_op *op, enum task_type type);
extern void async_register_poll_fn(int type, async_recv_t func);
extern int async_module_init(void);
extern int async_wake_job(ASYNC_JOB *job);
extern void async_free_poll_task(int id, bool is_cb);
extern int async_get_free_task(int *id);
extern void async_poll_task_free(void);
extern void async_init_op_done_v1(op_done_t *op_done);
extern void async_cleanup_op_done_v1(op_done_t *op_done);
#endif
