#!/bin/bash
##############################################################
## Copyright: Copyright (c) Huawei Technologies Co., Ltd. 2021-2021. All rights reserved.
## @Filename: ln-s.sh
## @Usage: sh ln-s.sh
##############################################################
set -e

asanFile="libasan.so.6"

if [ ! -d "lib" ]; then
	mkdir lib
fi

cd lib

if [ ! -f "libasan.so.6.0.0" ] && [ -f "/usr/lib64/libasan.so.6.0.0" ]; then
	cp /usr/lib64/libasan.so.6.0.0 libasan.so.6.0.0
fi

if [ ! -f "libasan.so.6.0.0" ] && [ -f "/usr/local/lib64/libasan.so.6.0.0" ]; then
	cp /usr/local/lib64/libasan.so.6.0.0 libasan.so.6.0.0
fi

if [ ! -f "$asanFile" ]; then
	ln -s libasan.so.6.0.0 libasan.so.6
fi