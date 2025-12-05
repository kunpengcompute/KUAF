#!/bin/sh
set -e
SRC_PATH=$(pwd)
COVER_FLAG=""

function zlib_sw_build()
{
    cd "${SRC_PATH}"/open_source
    if [ ! -d "${SRC_PATH}"/open_source/zlib ] && [ ! -f "${SRC_PATH}"/open_source/zlib.zip ]; then
        git clone -b v1.2.11 --depth 1  "https://github.com/madler/zlib.git"
    elif [ -f "${SRC_PATH}"/open_source/zlib.zip ]; then
        unzip zlib.zip
        mv zlib-* zlib
    fi
    cd "${SRC_PATH}"/open_source/zlib
    patch -Np1 < "${SRC_PATH}"/open_source/patch/kaezip_for_sec_CVE-2018-25032.patch
    patch -Np1 < "${SRC_PATH}"/open_source/patch/kaezip_for_sec_CVE-2022-37434.patch
    patch -Np1 < "${SRC_PATH}"/open_source/patch/zlib-1.2.11.patch
    chmod a+x configure
    ./configure --prefix=/usr/local/kuaf
    make clean && make
    make install
    echo "build zlib success"
}

function zlib_adapter_build()
{
    cd "${SRC_PATH}"/adapter/kuaf_comp/zlib_adapter
    make clean && make
    make install
    echo "build adapter success"
}

function kae_build()
{
    local source_dir="/usr/local/kaezip/lib"
    local target_dir="/usr/local/kuaf/lib"
    local pattern='^libkaezip\.so\.[0-9]+\.[0-9]+\.[0-9]+$'

    # create target dir if not exist yet
    mkdir -p "$target_dir"

    for file in "$source_dir"/libkaezip.so.*; do
        local filename=$(basename "$file")
        if [[ $filename =~ $pattern ]]; then
            cp -f "$file" "$target_dir/$filename"
            ln -sf "$filename" "$target_dir/libkaezip.so"
            ln -sf "$filename" "$target_dir/libkaezip.so.0"
            break
        fi
    done
}

securec_static_lib_build()
{
    cd ${SRC_PATH}
    if [ ! -d "${SRC_PATH}"/huawei_secure_c ]; then
        git clone https://gitee.com/openeuler/libboundscheck.git
        mv libboundscheck huawei_secure_c
    fi
    cd "${SRC_PATH}"/huawei_secure_c/src
    make lib
}

function kuaf_build()
{
    cd ${SRC_PATH}
    autoreconf -i
    cd ${SRC_PATH}/build
    ../configure --libdir=/usr/local/kuaf/lib $COVER_FLAG
    cd src
    make -j 16 
    make install

    cd ${SRC_PATH}

    if [ ! -d /etc/kuaf ]; then
        mkdir -p /etc/kuaf
    fi

    #if [ -z "$(ls -A /etc/kuaf)" ]; then
        cp -r config/* /etc/kuaf
    #fi
}

function kuaf_engine3_build()
{
    local source_dir="/usr/local/lib/engines-3.0"
    local target_dir="/usr/local/kuaf/lib"
    local pattern='^kae\.so\.[0-9]+\.[0-9]+\.[0-9]+$'

    openssl_install_path=$1
    if [ "$1" = "" ];then
        openssl_install_path=$(which openssl | awk -F'/bin' '{print $1}')
    fi
    openssl_install_path=${openssl_install_path%/}

    cd ${SRC_PATH}/adapter/kuaf_crypto
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
    autoreconf -i
    ./configure --libdir=/usr/local/kuaf/lib/ --enable-kae --enable-coverage --with-openssl_install_dir=$openssl_install_path
    make -j V=1
    make install
    
}


function kuaf_engine3_clean()
{
    cd ${SRC_PATH}/adapter/kuaf_crypto
    make uninstall
    make clean
    rm -rf /usr/local/kuaf/lib/kae.so*
    rm -rf /usr/local/kuaf/lib/libkae_crypto.so*
    # engine_log_clean
}

function kuaf_engine_build()
{
    local source_dir="/usr/local/lib/engines-1.1"
    local target_dir="/usr/local/kuaf/lib"
    local pattern='^kae\.so\.[0-9]+\.[0-9]+\.[0-9]+$'

    openssl_install_path=$1
    if [ "$1" = "" ];then
        openssl_install_path=$(which openssl | awk -F'/bin' '{print $1}')
    fi
    openssl_install_path=${openssl_install_path%/}

    cd ${SRC_PATH}/adapter/kuaf_crypto
    export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig
    autoreconf -i
    ./configure --libdir=/usr/local/kuaf/lib/ --enable-kae --enable-coverage --with-openssl_install_dir=$openssl_install_path
    make -j V=1
    make install
}


function kuaf_engine_clean()
{
    cd ${SRC_PATH}/adapter/kuaf_crypto
    make uninstall
    make clean
    rm -rf /usr/local/kuaf/lib/kae.so*
    rm -rf /usr/local/kuaf/lib/libkae_crypto.so*
    # engine_log_clean
}

function kuaf_clean()
{
    cd ${SRC_PATH}/build/src
    make uninstall
    make clean
    
    cd ${SRC_PATH}
    rm -rf build
    rm -rf /etc/kuaf/*
}

function help()
{
	echo "build KUAF"

	echo "sh build.sh all        -- install kuaf"
	echo "sh build.sh cleanup    -- uninstall kuaf"
}

function build_all_components()
{
    # 1. build zlib_sw
    zlib_sw_build

    # 2. move kae library to /usr/local/kuaf
    kae_build

    # 3. huawei_secure_c
    securec_static_lib_build

    # 4. build kuaf
    kuaf_build

    # 5. build zlib adapter
    zlib_adapter_build

    # 6. build engine adapter
    kuaf_engine_build
}

function clean_all_components()
{
    # clean zlib_sw
    cd "${SRC_PATH}"/open_source/
    rm -rf zlib

    # clean zlib adapter
    cd "${SRC_PATH}"/adapter/kuaf_comp/zlib_adapter
    make clean

    # clean kuaf
    kuaf_clean

    rm -rf /usr/local/kuaf/
}

main() {  
    case "$1" in  
        "kuaf")  
            if [ "$2" = "clean" ]; then  
                kuaf_clean
            elif [ "$2" = "cover" ]; then
                COVER_FLAG="--enable-cover"
                kuaf_build
            else  
                kuaf_build
            fi  
            ;;
        "kuaf_engine3")  
            if [ "$2" = "clean" ]; then  
                kuaf_engine3_clean
            else  
                kuaf_engine3_build
            fi  
            ;;
        "kuaf_engine")  
            if [ "$2" = "clean" ]; then  
                kuaf_engine_clean
            else  
                kuaf_engine_build
            fi  
            ;;     
        "all")
            if [ "$2" = "cover" ]; then
                COVER_FLAG="--enable-cover"
                build_all_components
            else
                build_all_components
            fi
            ;;
        "cleanup")
            clean_all_components
            ;;
        *)
    help
    ;;
    esac  
}  

main "$@"
exit $?