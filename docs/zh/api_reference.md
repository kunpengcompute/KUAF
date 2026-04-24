# API参考

## 接口列表

KUAF可供调用的接口如[**表 1** KUAF统一接口列表](#KUAF统一接口列表)所示。

**表 1** KUAF统一接口列表<a id="KUAF统一接口列表"></a>

|名称|说明|
|--|--|
|kuaf_ctx_scheduler_create|根据使用的算法类型和算法id创建ctx结构体。|
|kuaf_ctx_scheduler|决策层统一接口，获取业务需要的配置算法信息，调用配置文件模块读取配置文件，根据配置的调度算法策略调用具体的带宽处理调度/软硬算比例配置调度，调度决策信息记录在ctx结构体中。|
|kuaf_ctx_scheduler_free|释放结构体资源。|
|kuaf_ctx_process_sync|执行模块统一接口，根据ctx结构体信息，向下分发执行代码，调用对应算法的硬算或软算接口。|
|kuaf_ctx_end_process|kuaf_ctx_process_sync流程的后处理。|

>**须知：**
>
>- KUAF仅支持在鲲鹏平台下使用，为获得更优性能，接口内部不做完整入参校验，调用者请使用合法的入参，不合法的入参可能导致报错。
>- 在编写自定义函数时，建议**避免采用“kuaf”作为命名前缀**，以防止因全局符号重名导致的链接冲突。
>- 在处理原始数据大小未知的解压缩场景时，建议优先采用流式解压（Streaming Decompression）模式进行处理。若必须采用块解压（Block-based Decompression）方式，则需要预先估算并分配足够容量的目标缓冲区，否则当目标缓冲区尺寸不足时执行解压缩操作将触发未定义行为，导致程序出现不可预测的运行结果。

## 接口说明

### kuaf\_ctx\_scheduler\_create

**函数功能<a name="section816364604611"></a>**

根据使用的算法类型和算法id创建ctx结构体。

**函数定义<a name="section82574577467"></a>**

```c
scheduler_ctx* kuaf_ctx_scheduler_create(int alg_type, int alg_id);
```

**参数说明<a name="section8612145473311"></a>**

|参数名|描述|取值范围|输入/输出|
|--|--|--|--|
|alg_type|使用算法类型是解压缩。|ALG_TYPE_UNKNOWN（默认）、ALG_COMP（解压缩算法）|输入|
|alg_id|使用的算法id。|ALG_ID_UNKNOWN（默认）、ALG_ZLIB_DEFLATE （压缩算法id）、ALG_ZLIB_INFLATE（解压算法id）|输入|

**返回值<a name="zh-cn_topic_0000001201850423_section104851617202119"></a>**

- 成功：scheduler\_ctx\*指针。
- 失败：NULL。

### kuaf\_ctx\_scheduler

**函数功能<a name="section816364604611"></a>**

决策层统一接口负责获取业务需要的配置算法信息。调用配置文件模块读取配置文件。根据调度算法策略调用具体的带宽处理调度或软硬算比例配置调度。调度决策信息记录在ctx结构体中。

**函数定义<a name="section82574577467"></a>**

```c
int kuaf_ctx_scheduler(scheduler_ctx *sc_ctx)
```

**参数说明<a name="section8612145473311"></a>**

| 参数名 | 描述 | 取值范围 | 输入/输出 |
| --- | --- | --- | --- |
| sc_ctx | 调度结构体指针。 | 由kuaf_ctx_scheduler_create函数创建结构体。| 输入/输出 |

**返回值<a name="section1253745513711"></a>**

- 成功：KUAF\_SUCCESS。
- 失败：
    - KUAF\_NULL\_PTR\_ERROR：传入的sc\_ctx为空。
    - KUAF\_ALG\_NOT\_SUPPORT：传入不支持的算法类型。
    - KUAF\_ALGID\_ERROR：传入不支持的算法id。
    - KUAF\_STRA\_NOT\_SUPPORT：传入不支持的调度策略。

### kuaf\_ctx\_scheduler\_free

**函数功能<a name="section816364604611"></a>**

释放结构体资源。

**函数定义<a name="section82574577467"></a>**

```c
void kuaf_ctx_scheduler_free(scheduler_ctx *sc_ctx); 
```

**参数说明<a name="section8612145473311"></a>**

|参数名|描述|取值范围|输入/输出|
|--|--|--|--|
|sc_ctx|需要释放的调度结构体指针。|由kuaf_ctx_scheduler_create函数创建结构体。|输入|

**返回值<a name="section78901136457"></a>**

无。

### kuaf\_ctx\_process\_sync

**函数功能<a name="section816364604611"></a>**

执行模块统一接口，根据ctx结构体信息，向下分发执行代码，调用对应算法的硬算或软算接口。

**函数定义<a name="section82574577467"></a>**

```c
int kuaf_ctx_process_sync(scheduler_ctx *sc_ctx)
```

**参数说明<a name="section8612145473311"></a>**

|参数名|描述|取值范围|输入/输出|
|--|--|--|--|
|sc_ctx|调度结构体指针。|由kuaf_ctx_scheduler_create函数创建结构体。|输入|

**返回值<a name="section78901136457"></a>**

- 成功：对应调用硬算/软算法的返回值。
- 失败：
    - KUAF\_NULL\_PTR\_ERROR：传入的sc\_ctx指针为空。
    - KUAF\_ALG\_NOT\_SUPPORT：传入不支持的算法类型。

### kuaf\_ctx\_end\_process

**函数功能<a name="section19442111816541"></a>**

kuaf\_ctx\_process\_sync流程的后处理。

**函数定义<a name="section1144271855410"></a>**

```c
int kuaf_ctx_end_process(scheduler_ctx *sc_ctx);
```

**参数说明<a name="section1344211819546"></a>**

|参数名|描述|取值范围|输入/输出|
|--|--|--|--|
|sc_ctx|调度结构体指针。|由kuaf_ctx_scheduler_create函数创建结构体。|输入|

**返回值<a name="section144437185540"></a>**

- 成功：KUAF\_SUCCESS。
- 失败：KUAF\_NULL\_PTR\_ERROR：传入的结构体指针为空。

## 使用示例

本章节提供KUAF所有函数接口的使用示例。

1. 设置环境变量。

    ```bash
    export C_INCLUDE_PATH=/usr/local/kuaf/include:$C_INCLUDE_PATH
    export LD_LIBRARY_PATH=/usr/local/kuaf/lib/:/usr/local/lib:$LD_LIBRARY_PATH
    ```

2. 创建kuaf.c文件。
3. 按“i“进入编辑模式，在文件中添加以下内容。

    ```c
    #include "zlib.h"
    #include "kaezip.h"
    #include "kuaf_sc.h"
    #include "kuaf_alg.h"
    int main()
    {
        z_stream strm;
        strm.zalloc = (alloc_func)0;
        strm.zfree = (free_func)0;
        strm.opaque = (voidpf)0;
        // deflateInit2_（） 适配，创建调度结构体ctx，zlib函数本身入参数据封装成zlib_ctx结构体
        printf("kuaf_ctx_scheduler_create ctx here.\n");
        scheduler_ctx *ctx = kuaf_ctx_scheduler_create(ALG_COMP, ALG_ZLIB_DEFLATE);
        if (!ctx) {
            return Z_ERRNO;
        }
        printf("kuaf_zlib_ctx zlib_ctx malloc here.\n");
        kuaf_zlib_ctx *zlib_ctx =(kuaf_zlib_ctx *)malloc(sizeof(kuaf_zlib_ctx));
        if (!zlib_ctx) {
            kuaf_ctx_scheduler_free(ctx);
            return Z_ERRNO;
        }
        zlib_ctx->strm = &strm;
        zlib_ctx->flush = 0;
        ctx->data = (void *)zlib_ctx;
        printf("kuaf_ctx_scheduler here.\n");
        int ret = kuaf_ctx_scheduler(ctx);
        if (ret != KUAF_SUCCESS) {
            free(ctx->data);
            ctx->data = NULL;
            kuaf_ctx_scheduler_free(ctx);
            return Z_ERRNO;
        }
        /* Call KAE Hardware */
        if (ctx->dev_id != 0) {
            printf("kz_deflateInit2_ call here.\n");
        } else {
            printf("lz_deflateInit2_ call here.\n");
        }
        // deflate适配
        if (kz_get_devices()) {  // kae硬件可用性检测
            printf("kz_deflate call here.\n");
            kuaf_zlib_ctx *zlib_ctx = (kuaf_zlib_ctx *)ctx->data;
            // zlib_ctx->flush = flush;
            printf("kuaf_ctx_process_sync here \n");
            int ret = kuaf_ctx_process_sync(ctx);
            // ret < 0 means process failed
            if (ret < 0) {
                free(ctx->data);
                ctx->data = NULL;
                kuaf_ctx_scheduler_free(ctx);
                ret = Z_ERRNO;
                return -1;
            }
        } else {
            printf("lz_deflate call here.");
        }
        // deflateEnd适配
        if (kz_get_devices()) {  // kae硬件可用检测
            printf("deflateEnd kz_get_devices call here.\n");
            int ret = Z_OK;
            if (ctx->dev_id != 0) {
                kuaf_ctx_end_process(ctx);
            }
            if (ctx->data != NULL) {
                free(ctx->data);
                ctx->data = NULL;
            }
            kuaf_ctx_scheduler_free(ctx);
            printf("deflateEnd ctx free call here.\n");
            if (ret != Z_OK) {
                return ret;
            } else {
                printf("kz_deflateEnd call here.\n");
            }
        } else {
            printf("lz_deflateEnd call here.\n");
        }
        return 0;
    }
    ```

4. 按“Esc“键，输入:**wq!**，按“Enter“保存并退出编辑。
5. 编译kuaf.c文件，指定输出的可执行文件名称为kuaf。

    ```bash
    gcc kuaf.c -L/usr/local/kuaf/lib -lkuaf -lkaezip -lz -lz_sw -o kuaf
    ```

6. 执行可执行文件kuaf。

    ```bash
    ./kuaf
    ```

    输出如下运行结果信息。

    ```text
    kuaf_ctx_scheduler_create ctx here.
    kuaf_zlib_ctx zlib_ctx malloc here.
    kuaf_ctx_shcheduler here.
    kz_deflateInit2_ call here.
    kz_deflate call here.
    kuaf_ctx_process_sync here
    ```
