#!/bin/bash
# Copyright Huawei Technologies Co., Ltd. 2024-2025. All rights reserved.

set -e

execsuccess=0
execfailed=1

curd=$(pwd)
kuaf_dir=$curd/..
packagever=$(xmllint --xpath "//KUAF_INFO_VERSION/text()" ${kuaf_dir}/dependence.xml)
packagename=$(xmllint --xpath "//KUAF_INFO_COMPONENT_NAME/text()" ${kuaf_dir}/dependence.xml)
softname=$(xmllint --xpath "//KUAF_INFO_SOFTWARE_NAME/text()" ${kuaf_dir}/dependence.xml)
cpulimit=$(xmllint --xpath "//KUAF_INFO_CPU_LIMIT/text()" ${kuaf_dir}/dependence.xml)
excludedir="--exclude=$kuaf_dir/.git --exclude=$kuaf_dir/package/sourcecode --exclude=$kuaf_dir/.vscode --exclude=$kuaf_dir/test"
export KUAF_CPU_CHECK_ENABLE=${cpulimit}

function initialize()
{
    if [ -d "$curd/rpmbuild" ]; then
        rm -rf $curd/rpmbuild
    fi
    if [ -e "$curd/sourcecode/*.tar.gz" ]; then
        rm -rf $curd/sourcecode/*.tar.gz
    fi
    if [ -e "$curd/output/*.rpm" ]; then
        rm -rf $curd/output/*.rpm
    fi
    mkdir -p $curd/rpmbuild/{BUILD,RPMS,SOURCES,SPECS,SRPMS}
    if [ ! -d "$curd/output" ]; then
    mkdir -p "$curd/output"
    fi
    
    cd $curd/sourcecode
    tar -zcvf $softname-$packagever.tar.gz $excludedir $kuaf_dir
    cp $curd/sourcecode/$softname-$packagever.tar.gz $curd/rpmbuild/SOURCES/
    cp $curd/kuaf.spec $curd/rpmbuild/SPECS/
}

function pack_binary()
{
    cd $curd/rpmbuild/SPECS
    rpmbuild -bb --buildroot $curd/rpmbuild/BUILDROOT/ kuaf.spec
    if [ $? -ne $execsuccess ]; then
        exit $execfailed
    fi
    for file in "$curd/rpmbuild/RPMS/aarch64/$softname-$packagever"*.aarch64.rpm; do
        if [ -e "$file" ]; then
            cp "$file" $curd/output/
        fi
    done
}

function clean()
{
    rm -rf $curd/rpmbuild
    rm -rf $curd/sourcecode/*.tar.gz
    rm -rf $curd/output/*.rpm
}

      
function main()
{
    if [ "$1" == "" ] ;then
        initialize
        pack_binary
    elif [ "$1" == "clean" ] ;then
        clean
    fi
}

main $@