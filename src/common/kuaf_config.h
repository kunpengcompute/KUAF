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
#ifndef KUAF_CONFIG_H
#define KUAF_CONFIG_H

#define KUAF_CONFIG_FILE_NAME "kuaf"

#define KUAF_CONFIG_SECTION_NAME "KUAF_Section"
#define KUAF_CONFIG_LOG_LEVEL "log_level"
#define KUAF_CONFIG_BW_EXPIRED_INTERVAL "expired_interval"
#define KUAF_CONFIG_BW_UPDATE_INTERVAL "update_interval"

#define KUAF_CONFIG_KAE_FILE_NAME "kae"
#define KUAF_CONFIG_SCHEDULER "scheduler"
#define KUAF_CONFIG_SCHEDULER_BANDWIDTH "bandwidth"
#define KUAF_CONFIG_SCHEDULER_RATIO "ratio"
#define KUAF_CONFIG_SCHEDULER_PARALLEL "parallel"

int kuaf_config_init();

char *kuaf_config_get_str(char *dev_name, char *section_name, char *key);

int kuaf_config_get_int(char *dev_name, char *section_name, char *key);

void kuaf_config_free();

#endif