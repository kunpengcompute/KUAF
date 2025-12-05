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
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>

#include "kuaf_config.h"
#include "kuaf_log.h"

#define MAX_LEVEL_LEN 10
#define MAX_CONFIG_LEN 512

FILE *g_kuaf_debug_log_file = (FILE *)NULL;
pthread_mutex_t g_debug_file_mutex = PTHREAD_MUTEX_INITIALIZER;
int g_log_init_times;
int g_kuaf_log_level;
static int g_debug_file_ref_count;

const char *g_log_level[] = {
    "none", // default level
    "error",
    "warning",
    "info",
    "debug",
};

static void kuaf_set_conf_debuglevel(void)
{
    char *debuglev = (char *)NULL;

    debuglev = kuaf_config_get_str(KUAF_CONFIG_FILE_NAME, KUAF_CONFIG_SECTION_NAME, KUAF_CONFIG_LOG_LEVEL);
    if (!debuglev) {
        goto err;
    }

    for (int i = 0; i < sizeof(g_log_level) / sizeof(g_log_level[0]); i++) {
        if (strncmp(g_log_level[i], debuglev, strlen(debuglev) - 1) == 0) {
            g_kuaf_log_level = i;
            return;
        }
    }

err:
    g_kuaf_log_level = ERROR;
    if (debuglev != NULL) {
        debuglev = (char *)NULL;
    }
}

void kuaf_log_init(void)
{
    pthread_mutex_lock(&g_debug_file_mutex);
    kuaf_set_conf_debuglevel();
    if (!g_debug_file_ref_count && g_kuaf_log_level != NONE) {
        g_kuaf_debug_log_file = fopen(KUAF_DEBUG_FILE_PATH, "a+");
        if (g_kuaf_debug_log_file == NULL) {
            g_kuaf_debug_log_file = stderr;
            fprintf(stderr, "unable to open %s, %s\n", KUAF_DEBUG_FILE_PATH, strerror(errno));
        } else {
            g_debug_file_ref_count++;
        }
    }
    g_log_init_times++;
    pthread_mutex_unlock(&g_debug_file_mutex);
}

void kuaf_log_free(void)
{
    pthread_mutex_lock(&g_debug_file_mutex);
    g_log_init_times--;
    if (g_debug_file_ref_count && (g_log_init_times == 0)) {
        if (g_kuaf_debug_log_file != NULL) {
            fclose(g_kuaf_debug_log_file);
            g_debug_file_ref_count--;
            g_kuaf_debug_log_file = stderr;
        }
    }
    pthread_mutex_unlock(&g_debug_file_mutex);
}

void kuaf_save_log(FILE *src)
{
    size_t size = 0;
    char buf[1024] = {0};

    if (src == NULL) {
        return;
    }

    FILE *dst = fopen(KUAF_DEBUG_FILE_PATH_OLD, "w");

    if (dst == NULL) {
        return;
    }

    fseek(src, 0, SEEK_SET);
    while (1) {
        // 1024 is the size of buffer
        size = fread(buf, sizeof(char), 1024, src);
        fwrite(buf, sizeof(char), size, dst);
        if (!size) {
            break;
        }
    }

    fclose(dst);
}

void KUAF_LOG_LIMIT(int level, int times, int limit, const char *fmt, ...)
{
    struct tm *log_tm_p = (struct tm *)NULL;
    static unsigned long ulpre;
    static int should_print;
    va_list args2 = {0};

    if (level > g_kuaf_log_level) {
        return;
    }

    va_start(args2, fmt);
    time_t curr = time((time_t *)NULL);
    if (difftime(curr, ulpre) > limit) {
        should_print = times;
    }
    if (should_print <= 0) {
        should_print = 0;
    }
    if (should_print-- > 0) {
        log_tm_p = (struct tm *)localtime(&curr);
        flock(g_kuaf_debug_log_file->_fileno, LOCK_EX);
        pthread_mutex_lock(&g_debug_file_mutex);
        fseek(g_kuaf_debug_log_file, 0, SEEK_END);
        if (log_tm_p != NULL) {
            // 1900 is the year of the Gregorian calendar
            fprintf(g_kuaf_debug_log_file,
                "[%4d-%02d-%02d %02d:%02d:%02d][%s][%s:%d:%s()] ",
                (1900 + log_tm_p->tm_year),
                (1 + log_tm_p->tm_mon),
                log_tm_p->tm_mday,
                log_tm_p->tm_hour,
                log_tm_p->tm_min,
                log_tm_p->tm_sec,
                g_log_level[level],
                __FILE__,
                __LINE__,
                __func__);
        } else {
            fprintf(g_kuaf_debug_log_file, "[%s][%s:%d:%s()] ", g_log_level[level], __FILE__, __LINE__, __func__);
        }
        vfprintf(g_kuaf_debug_log_file, fmt, args2);
        fprintf(g_kuaf_debug_log_file, "\n");
        if (ftell(g_kuaf_debug_log_file) > KUAF_LOG_MAX_SIZE) {
            kuaf_save_log(g_kuaf_debug_log_file);
            if (ftruncate(g_kuaf_debug_log_file->_fileno, 0) != 0) {
                perror("ftruncate failed.\n");
                pthread_mutex_unlock(&g_debug_file_mutex);
                flock(g_kuaf_debug_log_file->_fileno, LOCK_UN);
                return;
            }
            fseek(g_kuaf_debug_log_file, 0, SEEK_SET);
        }
        pthread_mutex_unlock(&g_debug_file_mutex);
        flock(g_kuaf_debug_log_file->_fileno, LOCK_UN);
        ulpre = (unsigned long)time((time_t *)NULL);
    }
    va_end(args2);
}

static int need_debug(void)
{
    if (g_kuaf_log_level >= DEBUG) {
        return 1;
    } else {
        return 0;
    }
}

void dump_data(const char *name, unsigned char *buf, unsigned int len)
{
    unsigned int i;

    if (need_debug()) {
        KUAF_DEBUG("DUMP ==> %s", name);
        // 8 is the number of bytes per line
        for (i = 0; i + 8 <= len; i += 8) {
            KUAF_DEBUG("0x%llx: \t%02x %02x %02x %02x %02x %02x %02x %02x",
                (unsigned long long)(buf + i),
                *(buf + i),
                *(buf + i + 1),
                *(buf + i + 2),
                *(buf + i + 3),
                *(buf + i + 4),
                *(buf + i + 5),
                *(buf + i + 6),
                *(buf + i + 7));
        }

        // 8 is the number of bytes per line
        if (len % 8) {
            KUAF_DEBUG("0x%llx: \t", (unsigned long long)(buf + i));
            for (; i < len; i++) {
                KUAF_DEBUG("%02x ", buf[i]);
            }
        }
    }
}

void dump_bd(unsigned int *bd, unsigned int len)
{
    unsigned int i;

    if (need_debug()) {
        for (i = 0; i < len; i++) {
            KUAF_DEBUG("Word[%u] 0x%08x", i, bd[i]);
        }
    }
}