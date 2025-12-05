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
#ifndef KUAF_LOG_H
#define KUAF_LOG_H

#include <sys/file.h>
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <pthread.h>

#define LOG_LEVEL_CONFIG NONE
#define KUAF_DEBUG_FILE_PATH "/var/log/kuaf.log"
#define KUAF_DEBUG_FILE_PATH_OLD "/var/log/kuaf.log.old"
#define KUAF_DEBUG_FILE_PATH_LOCK "/var/log/.kuaf.log.lock"
#define KUAF_LOG_MAX_SIZE 209715200
#define LOG_PRINT_NUM 20

extern FILE *g_kuaf_debug_log_file;
extern pthread_mutex_t g_debug_file_mutex;
extern const char *g_log_level[];
extern int g_kuaf_log_level;

enum KUAF_LOG_LEVEL {
    NONE = 0,
    ERROR,
    WARNING,
    INFO,
    DEBUG,
};

void kuaf_log_init(void);
void kuaf_log_free(void);
void kuaf_save_log(FILE *src);

#define KUAF_WRITE_LOG(LEVEL, fmt, args...)                                        \
    do {                                                                           \
        if (LEVEL > g_kuaf_log_level) {                                            \
            break;                                                                 \
        }                                                                          \
        struct tm *log_tm_p = NULL;                                                \
        time_t timep = time((time_t *)NULL);                                       \
        log_tm_p = localtime(&timep);                                              \
        int log_lock_fd = open(KUAF_DEBUG_FILE_PATH_LOCK, O_CREAT | O_RDWR, 0666); \
        if (log_lock_fd < 0) {                                                     \
            perror("[kuaf]:open log lock file error");                             \
            break;                                                                 \
        }                                                                          \
        if (flock(log_lock_fd, LOCK_EX) != 0) {                                    \
            perror("[kuaf]:flock error, check /var/log/.kuaf.log.lock");           \
            close(log_lock_fd);                                                    \
            break;                                                                 \
        }                                                                          \
        pthread_mutex_lock(&g_debug_file_mutex);                                   \
        fseek(g_kuaf_debug_log_file, 0, SEEK_END);                                 \
        if (log_tm_p != NULL) {                                                    \
            fprintf(g_kuaf_debug_log_file,                                         \
                "[%4d-%02d-%02d %02d:%02d:%02d][%s][%s:%d:%s()] " fmt "\n",        \
                (1900 + log_tm_p->tm_year),                                        \
                (1 + log_tm_p->tm_mon),                                            \
                log_tm_p->tm_mday,                                                 \
                log_tm_p->tm_hour,                                                 \
                log_tm_p->tm_min,                                                  \
                log_tm_p->tm_sec,                                                  \
                g_log_level[LEVEL],                                                \
                __FILE__,                                                          \
                __LINE__,                                                          \
                __func__,                                                          \
                ##args);                                                           \
                fflush(g_kuaf_debug_log_file);                                     \
        } else {                                                                   \
            fprintf(g_kuaf_debug_log_file,                                         \
                "[%s][%s:%d:%s()] " fmt "\n",                                      \
                g_log_level[LEVEL],                                                \
                __FILE__,                                                          \
                __LINE__,                                                          \
                __func__,                                                          \
                ##args);                                                           \
                fflush(g_kuaf_debug_log_file);                                     \
        }                                                                          \
        if (ftell(g_kuaf_debug_log_file) > KUAF_LOG_MAX_SIZE) {                    \
            kuaf_save_log(g_kuaf_debug_log_file);                                  \
            if (ftruncate(g_kuaf_debug_log_file->_fileno, 0))                      \
                ;                                                                  \
            fseek(g_kuaf_debug_log_file, 0, SEEK_SET);                             \
        }                                                                          \
        pthread_mutex_unlock(&g_debug_file_mutex);                                 \
        flock(log_lock_fd, LOCK_UN);                                               \
        close(log_lock_fd);                                                        \
    } while (0)

void KUAF_LOG_LIMIT(int level, int times, int limit, const char *fmt, ...);

#define KUAF_ERROR(fmt, args...) KUAF_WRITE_LOG(ERROR, fmt, ##args)
#define KUAF_WARN(fmt, args...) KUAF_WRITE_LOG(WARNING, fmt, ##args)
#define KUAF_INFO(fmt, args...) KUAF_WRITE_LOG(INFO, fmt, ##args)
#define KUAF_DEBUG(fmt, args...) KUAF_WRITE_LOG(DEBUG, fmt, ##args)

#define KUAF_ERROR_LIMIT(fmt, args...) KUAF_LOG_LIMIT(ERROR, 3, 1, fmt, ##args)
#define KUAF_WARN_LIMIT(fmt, args...) KUAF_LOG_LIMIT(WARNING, 3, 1, fmt, ##args)
#define KUAF_INFO_LIMIT(fmt, args...) KUAF_LOG_LIMIT(INFO, 3, 1, fmt, ##args)
#define KUAF_DEBUG_LIMIT(fmt, args...) KUAF_LOG_LIMIT(DEBUG, 3, 1, fmt, ##args)

void dump_data(const char *name, unsigned char *buf, unsigned int len);
void dump_bd(unsigned int *bd, unsigned int len);

#endif
