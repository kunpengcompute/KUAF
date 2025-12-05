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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <ctype.h>
#include "securec.h"

#include "kuaf_config.h"

#define MAX_FILES 100
#define MAX_SECTIONS 10
#define MAX_KEYS 10
#define MAX_LINE_LENGTH 256
#define MAX_NAME_LENGTH 50
#define MAX_VALUE_LENGTH 100
#define KUAF_CONFIG_PATH "/etc/kuaf"

typedef struct {
    char key[MAX_NAME_LENGTH];
    char value[MAX_VALUE_LENGTH];
} KeyValue;

typedef struct {
    char section[MAX_NAME_LENGTH];
    KeyValue keys[MAX_KEYS];
    int key_count;
} Section;

typedef struct {
    char filename[MAX_NAME_LENGTH];
    Section sections[MAX_SECTIONS];
    int section_count;
} KUAFConfigFile;

static KUAFConfigFile kuaf_configs[MAX_FILES];
static int cfg_file_count = 0;

// 去除字符串首尾的空白字符
static char *trim_whitespace(char *str)
{
    char *end;

    while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') {
        str++;
    }

    if (*str == 0) {
        return str;
    }

    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }

    *(end + 1) = '\0';

    return str;
}

// 查找指定文件的KUAFConfigFile结构
static KUAFConfigFile *find_cfg_file(char *filename)
{
    for (int i = 0; i < cfg_file_count; i++) {
        if (strcmp(kuaf_configs[i].filename, filename) == 0) {
            return &kuaf_configs[i];
        }
    }
    return NULL;
}

// 查找指定节的Section 结构
static Section *find_section(KUAFConfigFile *cfg_file, char *section)
{
    for (int i = 0; i < cfg_file->section_count; i++) {
        if (strcmp(cfg_file->sections[i].section, section) == 0) {
            return &cfg_file->sections[i];
        }
    }
    return NULL;
}

// 查找指定键的KeyValue 结构
static KeyValue *find_key_value(Section *sec, char *key)
{
    for (int i = 0; i < sec->key_count; i++) {
        if (strcmp(sec->keys[i].key, key) == 0) {
            return &sec->keys[i];
        }
    }
    return NULL;
}

// 解析INI格式文件
static void parse_cfg_file(char *filepath, char *filename)
{
    FILE *file = fopen(filepath, "r");
    size_t len = strlen(filename);
    if (!file) {
        perror("无法打开文件");
        return;
    }

    if (cfg_file_count >= MAX_FILES) {
        fprintf(stderr, "已达到最大文件数量限制\n");
        fclose(file);
        return;
    }

    KUAFConfigFile *cfg_file = &kuaf_configs[cfg_file_count++];

    errno_t ret;
    if (len > 4 && strcmp(filename + len - 4, ".cnf") == 0) {
        ret = strncpy_s(cfg_file->filename, MAX_NAME_LENGTH, filename, len - 4);
        if (ret != EOK) {
            fclose(file);
            perror("strncpy_s error occurred.\n");
            return;
        }
        cfg_file->filename[len - 4] = '\0';
    } else {
        fclose(file);
        perror("文件名有误\n");
        return;
    }

    cfg_file->section_count = 0;

    char line[MAX_LINE_LENGTH];
    Section *current_section = NULL;

    while (fgets(line, sizeof(line), file) != NULL) {
        char *trimmed_line = trim_whitespace(line);
        if (trimmed_line[0] == ';' || trimmed_line[0] == '#' || trimmed_line[0] == '\0') {
            continue;
        } else if (trimmed_line[0] == '[') {
            char *end = strchr(trimmed_line, ']');
            if (end) {
                *end = '\0';
                if (cfg_file->section_count < MAX_SECTIONS) {
                    current_section = &cfg_file->sections[cfg_file->section_count++];
                    ret = strncpy_s(current_section->section, MAX_NAME_LENGTH, trimmed_line + 1, MAX_NAME_LENGTH - 1);
                    if (ret != EOK) {
                        fclose(file);
                        perror("strncpy_s error occurred.\n");
                        return;
                    }
                    current_section->key_count = 0;
                }
            }
        } else if (current_section) {
            char *equals = strchr(trimmed_line, '=');
            if (equals) {
                *equals = '\0';
                char *key = trim_whitespace(trimmed_line);
                char *value = trim_whitespace(equals + 1);
                if (current_section->key_count < MAX_KEYS) {
                    KeyValue *kv = &current_section->keys[current_section->key_count++];
                    errno_t ret_key = strncpy_s(kv->key, MAX_NAME_LENGTH, key, MAX_NAME_LENGTH - 1);
                    errno_t ret_val = strncpy_s(kv->value, MAX_VALUE_LENGTH, value, MAX_VALUE_LENGTH - 1);
                    if (ret_key != EOK || ret_val != EOK) {
                        fclose(file);
                        perror("strncpy_s error occurred.\n");
                        return;
                    }
                }
            }
        }
    }
    fclose(file);
}

// 读取目录下的所有.cnf文件并解析
static int load_kuaf_configs(char *directory)
{
    DIR *dir;
    struct dirent *entry;

    if ((dir = opendir(directory)) == NULL) {
        perror("无法打开目录");
        return 0;
    }

    ssize_t len;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG) {
            char *ext = strrchr(entry->d_name, '.');
            if (ext && strcmp(ext, ".cnf") == 0) {
                char filepath[PATH_MAX];
                len = snprintf_s(filepath, sizeof(filepath), sizeof(filepath) - 1, "%s/%s", directory, entry->d_name);
                if (len < 0) {
                    closedir(dir);
                    perror("strncpy_s error occurred.\n");
                    return 0;
                }
                parse_cfg_file(filepath, entry->d_name);
            }
        }
    }
    closedir(dir);
    return 1;
}

int kuaf_config_init()
{
    if (cfg_file_count == 0) {
        if (!load_kuaf_configs(KUAF_CONFIG_PATH)) {
            printf("load kuaf config failed\n");
            return 0;
        }
    }
    return 1;
}

// 获取指定文件、节和键的值
char *kuaf_config_get_str(char *filename, char *section, char *key)
{
    KUAFConfigFile *cfg_file = find_cfg_file(filename);
    if (!cfg_file) {
        return NULL;
    }

    Section *sec = find_section(cfg_file, section);
    if (!sec) {
        return NULL;
    }

    KeyValue *kv = find_key_value(sec, key);
    if (!kv) {
        return NULL;
    }

    return kv->value;
}

int kuaf_config_get_int(char *filename, char *section, char *key)
{
    char *str_value = kuaf_config_get_str(filename, section, key);
    if (str_value) {
        return atoi(str_value);
    }
    return -1;
}

void kuaf_config_free()
{
    return;
}