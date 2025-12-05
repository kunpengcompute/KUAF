#!/bin/bash
##############################################################
## Copyright: Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
## @Filename: download.sh
## @Usage:    sh download.sh download googletest source code
##############################################################
set -e

curd=$(pwd)
kuaf_test=$curd/..
root_path=$curd/../..

if [ -d ${kuaf_test}/googletest ]
then
    echo -e "\033[32m The googletest directory already exists and does not need to be downloaded. \033[0m"
    exit 0
fi

cd ${kuaf_test}
git clone https://github.com/google/googletest.git