# 安装指南

## 已验证环境

为保证您可以顺利安全地使用KUAF，请确保所使用的环境信息在已验证环境范围内。

|操作系统|CPU类型|编译器|
|--|--|--|
|openEuler 22.03 LTS SP3|鲲鹏920系列处理器|GCC 10.3.1|


>**须知：** 
>KUAF目前只支持鲲鹏920系列处理器调度。




## 源码安装和卸载

请先安装KAE，随后按照本章节步骤安装KUAF。安装完成后，需运行指定脚本编译构建zlib库（该过程会从GitHub仓库下载源码，请确保网络可正常访问GitHub）。

**安装步骤<a name="section68113416198"></a>**

1. 请参见《[鲲鹏加速引擎 用户指南](https://www.hikunpeng.com/document/detail/zh/kunpengaccel/kae/usermanual/kunpengaccel_16_0002.html)》对KAE进行安装。
2. 通过以下命令下载KUAF源码将KUAF源码包拷贝到自定义路径下并解压。

    ```
    git clone https://gitcode.com/boostkit/KUAF.git
    ```

3. 一键安装所有模块。

    ```
    cd KUAF
    sh build.sh all
    ```

4. 通过配置OpenSSL的openssl.cnf文件使能KUAF。

    新建openssl.cnf需要添加如下配置信息：

    ```
    openssl_conf=openssl_def 
    [openssl_def]
    engines=engine_section 
    [engine_section] 
    kae=kae_section 
    [kae_section] 
    engine_id=kae
    dynamic_path=/usr/local/kuaf/lib/libkae.so 
    default_algorithms=ALL 
    init=1
    ```

    设置OPENSSL\_CONF环境变量：

    ```
    export OPENSSL_CONF=/home/app/openssl.cnf  #该路径为openssl.cnf存放路径
    ```

**安装后验证<a name="section5820418197"></a>**

1. 设置环境变量。

    ```
    export LD_LIBRARY_PATH=/usr/local/kuaf/lib:/usr/local/lib:/usr/local/lib/engines-1.1$LD_LIBRARY_PATH
    ```

2. 查看“/usr/local/kuaf/lib“目录下so情况。

    ```
    ll /usr/local/kuaf/lib
    ```

    回显信息中有libkuaf.so、libz\_sw.so、libkaezip.so、libkae.so存在说明安装成功。

    ```
    total 1664
    -rwxr-xr-x. 1 root root   1250 Nov  6 19:24 libkae.la
    -rwxr-xr-x. 1 root root 364432 Nov  6 19:24 libkae.so
    lrwxrwxrwx. 1 root root     18 Nov  5 15:03 libkaezip.so -> libkaezip.so.2.0.4
    lrwxrwxrwx. 1 root root     18 Nov  5 15:03 libkaezip.so.0 -> libkaezip.so.2.0.4
    -rwxr-xr-x. 1 root root 215640 Nov  5 15:03 libkaezip.so.2.0.4
    -rw-r--r--. 1 root root 604300 Nov  5 15:32 libkuaf.a
    -rwxr-xr-x. 1 root root   1121 Nov  5 15:32 libkuaf.la
    lrwxrwxrwx. 1 root root     16 Nov  5 15:32 libkuaf.so -> libkuaf.so.1.0.0
    lrwxrwxrwx. 1 root root     16 Nov  5 15:32 libkuaf.so.1 -> libkuaf.so.1.0.0
    -rwxr-xr-x. 1 root root 346496 Nov  5 15:32 libkuaf.so.1.0.0
    lrwxrwxrwx. 1 root root     34 Nov  5 15:03 libz.so -> /usr/local/kuaf/lib/libz.so.1.2.11
    lrwxrwxrwx. 1 root root     34 Nov  5 15:03 libz.so.1 -> /usr/local/kuaf/lib/libz.so.1.2.11
    -rwxr-xr-x. 1 root root  67696 Nov  5 15:03 libz.so.1.2.11
    -rw-r--r--. 1 root root 141674 Nov  5 15:03 libz_sw.a
    lrwxrwxrwx. 1 root root     17 Nov  5 15:03 libz_sw.so -> libz_sw.so.1.2.11
    lrwxrwxrwx. 1 root root     17 Nov  5 15:03 libz_sw.so.1 -> libz_sw.so.1.2.11
    -rwxr-xr-x. 1 root root 143904 Nov  5 15:03 libz_sw.so.1.2.11
    ```

    安装路径（默认路径是“/usr/local/kuaf“）下生成相应文件，其中，“include“文件夹包含KUAF库的头文件，“lib“文件夹包含了KUAF的动态库文件。

**卸载KUAF<a name="section245327516"></a>**

若不再需要使用KUAF，可卸载KUAF。卸载KUAF将会影响您正在执行的业务流，建议先停止正在执行的业务流再进行卸载操作。

1. 执行脚本卸载KUAF。

    ```
    sh build.sh cleanup
    ```

2. 查看“/usr/local/kuaf“目录，确认安装目录“/usr/local/kuaf“已被删除。

    ```
    ll /usr/local/kuaf
    ```

    回显信息中显示找不到该目录，则安装目录删除成功。

3. 删除OPENSSL\_CONF环境变量。

    ```
    unset OPENSSL_CONF
    ```


