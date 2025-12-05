#!/bin/bash
set -e
CUR_PATH=$(pwd)

# Check if /usr/local/kuaf exists
if [ ! -d "/usr/local/kuaf" ]; then
    echo "Error: Directory /usr/local/kuaf not found"
    exit 1
fi

# Clone zlib repository
git clone --depth 1 -b v1.2.11 https://github.com/madler/zlib.git
if [ $? -ne 0 ]; then
    echo "Error: Failed to clone zlib repository"
    exit 1
fi

# build zlib library
cd zlib
if [ $? -ne 0 ]; then
    echo "Error: Failed to enter zlib directory"
    exit 1
fi

echo "Performing zlib build operations..."
patch -Np1 < "${CUR_PATH}"/patch/kaezip_for_sec_CVE-2018-25032.patch
patch -Np1 < "${CUR_PATH}"/patch/kaezip_for_sec_CVE-2022-37434.patch
patch -Np1 < "${CUR_PATH}"/patch/zlib-1.2.11.patch
./configure --prefix=/usr/local/kuaf
make clean && make
make install

cd "${CUR_PATH}"
rm -rf zlib

# Check for libkaezip directory
if [ ! -d "/usr/local/kaezip" ]; then
    echo "Error: Directory /usr/local/kaezip not found"
    exit 1
fi
source_dir="/usr/local/kaezip/lib"
target_dir="/usr/local/kuaf/lib"
pattern='^libkaezip\.so\.[0-9]+\.[0-9]+\.[0-9]+$'
for file in "$source_dir"/libkaezip.so.*; do
    filename=$(basename "$file")
    if [[ $filename =~ $pattern ]]; then
        cp -f "$file" "$target_dir/$filename"
        ln -sf "$filename" "$target_dir/libkaezip.so"
        ln -sf "$filename" "$target_dir/libkaezip.so.0"
        break
    fi
done

echo "KUAF installed"