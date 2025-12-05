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
#include <string.h>
#include "kuaf_alg.h"

char* kuaf_alg_get_name(int alg_id)
{
    for (int i = 0; alg_map[i].name != NULL; i++) {
        if (alg_map[i].id == alg_id) {
            return alg_map[i].name;
        }
    }
    return NULL;
}

int kuaf_alg_get_id(char *name)
{   
    if (name == NULL) {
        return ALG_ID_UNKNOWN;
    }
    for (int i = 0; alg_map[i].name != NULL; i++) {
        if (strcmp(alg_map[i].name, name) == 0) {
            return alg_map[i].id;
        }
    }
    return ALG_ID_UNKNOWN;
}

int kuaf_alg_check(int alg_type, int alg_id)
{
    for (size_t i = 0; i < sizeof(id_type_map) / sizeof(id_type_map[0]); ++i) {
        if (id_type_map[i].id == alg_id) {
            return (id_type_map[i].type == alg_type);
        }
    }
    return 0;
}