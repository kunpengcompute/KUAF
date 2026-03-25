# 快速入门<a name="ZH-CN_TOPIC_0000002518596366"></a>

本文档提供了鲲鹏统一加速框架的功能描述和配置步骤，旨在帮助用户快速熟悉鲲鹏统一加速框架的使用。

KUAF（Kunpeng Unified Acceleration Framework，鲲鹏统一加速框架）是鲲鹏自研的统一加速框架，是一个实现鲲鹏加速引擎（KAE，Kunpeng Accelerator Engine）硬件协同软压缩库和软加解密库加速的解决方案。

KUAF具备调度KAE硬件加速器，硬加速zlib的压缩/解压和OpenSSL的加解密功能。KUAF支持的调度策略如[**表 1** KUAF调度策略](#KUAF调度策略) KUAF调度策略]所示。

**表 1** KUAF调度策略<a id="KUAF调度策略"></a>

|序号|调度策略|策略描述|
|--|--|--|
|1|硬件带宽调度策略|统计硬件带宽使用率，硬件带宽达到上限时切换成软优化库执行。|
|2|软硬算比例调度策略|配置软硬件调度比例，按比例随机分发执行策略。|


**算法支持<a name="section0850650192113"></a>**

- 加解密
    - 支持非对称算法RSA，同步模式
    - 支持非对称算法SM2，同步模式

- zlib：

    支持zlib开源库中的deflate/inflate压缩/解压相关的接口
    
**使用步骤**

- 配置
    - 配置KUAF的配置文件
    - /etc/kuaf/kuaf.cnf默认配置文件如下, log_level为日志等级，可配置none、debug、error、warning、info；expired_interval为硬件带宽调度策略的数据过期时间，单位ms；update_interval为硬件带宽调度策略的守护线程更新数据的时间间隔，单位ms。
        ```
        [KUAF_Section]
        log_level=none
        expired_interval=500
        update_interval=50
        ```
    - /etc/kuaf/kae.cnf默认配置文件如下，ZLIB-DEFLATE是Zlib压缩的调度配置项，ZLIB-INFLATE是Zlib解压的调度配置项。scheduler为调度策略，可配置为bandwidth和ratio；bandwidth策略的数值表示给硬算分配的带宽，达到带宽后切软算，ratio策略的数值表示走硬算的比例。
        ```
        [ZLIB-DEFLATE]
        scheduler=bandwidth
        bandwidth=3072

        [ZLIB-INFLATE]
        scheduler=ratio
        ratio=60%
        ```
- 测试
    - KUAF提供了一个性能测试工具，可以初步评估Zlib/crypto性能。
    - 解压缩
        ```
        cd test/perftest/zlib_perf
        make

        #参数说明：
            -m: multi process
            -l: stream length(KB)
            -w: windowBits
            -v: compress level(1~9)
            -f: input  filename(-l useless if this work)
            -o: output filename
            -n: loop times
            -t: run seconds
            -i: initialize a new compression stream for each task
            -d: compress or decompress

        #例子: 
        taskset -c 0-63 ./zip_perf -m 63 -n 1000 -i -f itemdata  #拉起63个子进程，每个子进程做1000次压缩任务，每次任务都初始化新的流，输入数据为itemadata
        taskset -c 0-63 ./zip_perf -m 63 -t 10 -i -f itemdata.gz -d  #拉起63个子进程，做解压任务，运行10秒，每次任务都初始化新的流，输入数据为itemadata.gz
        
        ```
   - 加解密
        ```
        cd test/perftest/crypto_perf
        make

        #测试RSA算法:
        ./rsa_perf
        
        #测试SM2算法：
        ./sm2_perf
   
        ```