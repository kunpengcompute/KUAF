# 快速入门<a name="ZH-CN_TOPIC_0000002518596366"></a>

本文档提供了鲲鹏统一加速框架的功能描述和配置步骤，旨在帮助用户快速熟悉鲲鹏统一加速框架的使用。

## 前提条件

使用前请参见《[安装指南](./installation_guide.md)》完成鲲鹏统一加速框架的安装。
    
## 配置文件

**配置KUAF配置文件kuaf.cnf**

配置路径：/etc/kuaf/kuaf.cnf，默认配置内容如下。配置参数说明如[**表 1** kuaf.cnf文件配置选项说明](#kuaf.cnf文件配置选项说明)所示。

```bash
[KUAF_Section]
log_level=none
expired_interval=500
upda
```

**表 1** kuaf.cnf文件配置选项说明<a id="kuaf.cnf文件配置选项说明"></a>

| 配置项 | 配置项说明 |
| --- | --- |
| log_level | 日志等级。可配置为none、debug、error、warning、info。 |
| expired_interval | 硬件带宽调度策略的数据过期时间，单位ms。 |
| update_interval | 硬件带宽调度策略的守护线程更新数据的时间间隔，单位ms。 |

**配置KAE配置文件kae.cnf**

配置路径：/etc/kuaf/kae.cnf，默认配置文件如下。配置参数说明如[**表 2** kae.cnf文件配置选项说明](#kae.cnf文件配置选项说明)所示。

```bash
[ZLIB-DEFLATE]
scheduler=bandwidth
bandwidth=3072

[ZLIB-INFLATE]
scheduler=ratio
ratio=60%
```

**表 2** kae.cnf文件配置选项说明<a id="kae.cnf文件配置选项说明"></a>

| 配置项 | 配置项说明 |
| --- | --- |
| ZLIB-DEFLATE | zlib压缩的调度配置项。 |
| ZLIB-INFLATE | zlib解压的调度配置项。 |
| scheduler | 调度策略，可配置为bandwidth和ratio。</br>bandwidth：策略的数值表示给硬算分配的带宽，达到带宽后切软算。</br>ratio：策略的数值表示走硬算的比例。 |

## 测试步骤

KUAF提供了一个性能测试工具，可以初步评估zlib/crypto性能。

**解压缩**

1. 进入测试工具目录。

    ```bash
    cd test/perftest/zlib_perf
    make
    ```

2. 测试命令。命令参数说明如[**表 3** 解压缩命令参数说明](#解压缩命令参数说明)所示。

   - 压缩：以启动63个子进程，每个子进程执行1000次压缩任务，每次任务都初始化新的流，输入数据为itemadata为例。

      ```bash
      taskset -c 0-63 ./zip_perf -m 63 -n 1000 -i -f itemdata
      ```

   - 解压缩：以启动63个子进程，执行解压任务，运行10秒，每次任务都初始化新的流，输入数据为itemadata.gz为例。

      ```bash
      taskset -c 0-63 ./zip_perf -m 63 -t 10 -i -f itemdata.gz -d
      ```

    **表 3** 解压缩命令参数说明<a id="解压缩命令参数说明"></a>

    | 参数 | 说明 |
    | --- | --- |
    | -m | 表示多进程。 |
    | -l | 数据流长度，单位KB。 |
    | -w | windowBits |
    | -v | 压缩级别（1~9）。 |
    | -f | 输入文件名称（-l在此情况下无效）。 |
    | -o | 输出文件名称。 |
    | -n | 循环次数。 |
    | -t | 运行时间，单位s。 |
    | -i | 为每个任务初始化一个新的压缩流。 |
    | -d | 压缩或解压缩。 |

**加解密**

1. 进入测试工具目录。

    ```bash
    cd test/perftest/crypto_perf
    make
    ```

2. 测试命令。

   - 测试RSA算法。

      ```bash
      ./rsa_perf
      ```

   - 测试SM2算法。

      ```bash
      ./sm2_perf
      ```
