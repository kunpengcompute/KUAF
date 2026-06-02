# API Reference

## APIs

[**Table 1**](#KUAF-APIs) lists APIs provided by KUAF.

**Table 1** KUAF APIs<a id="KUAF-APIs"></a>

|API|Description|
|--|--|
|kuaf_ctx_scheduler_create|Creates a **ctx** structure according to the algorithm type and algorithm ID.|
|kuaf_ctx_scheduler|Acts as a unified API at the decision-making layer. It obtains information about the algorithm required by the service and invokes the configuration file module to read configuration files. Then, according to the configured scheduling policy, it invokes bandwidth-based scheduling or ratio-based scheduling, and records scheduling decision-making information in the **ctx** structure.|
|kuaf_ctx_scheduler_free|Releases the structure resource.|
|kuaf_ctx_process_sync|Acts as a unified API of the execution module. It distributes execution code downwards and calls the hardware or software computing APIs of the algorithm based on information in the **ctx** structure.|
|kuaf_ctx_end_process|Performs post-processing after kuaf_ctx_process_sync.|

>**Notice**:
>
>- KUAF is available on the Kunpeng platform only. To achieve better performance, complete input parameter verification is not performed in KUAF APIs. Use valid input parameters, and invalid input parameters may cause errors.
>- When writing a user-defined function, you are advised to **avoid using "kuaf" as the naming prefix** to prevent link conflicts caused by name collision with the global symbol.
>- In decompression scenarios where the size of raw data is unknown, you are advised to use the streaming decompression mode. If block-based decompression is required, you need to estimate the data size and allocate a target buffer with sufficient space. Otherwise, decompression operations will trigger undefined behavior when the target buffer space is insufficient, resulting in unpredictable program execution results.

## API Description

### kuaf\_ctx\_scheduler\_create

**Function Usage<a name="section816364604611"></a>**

Creates a **ctx** structure according to the algorithm type and algorithm ID.

**Function Syntax<a name="section82574577467"></a>**

```c
scheduler_ctx* kuaf_ctx_scheduler_create(int alg_type, int alg_id);
```

**Parameters<a name="section8612145473311"></a>**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|alg_type|Algorithm type, which is decompression.|ALG_TYPE_UNKNOWN (default), ALG_COMP (decompression)|Input|
|alg_id|ID of the algorithm to be used.|ALG_ID_UNKNOWN (default), ALG_ZLIB_DEFLATE (compression algorithm ID), ALG_ZLIB_INFLATE (decompression algorithm ID)|Input|

**Return Value<a name="en-us_topic_0000001201850423_section104851617202119"></a>**

- Success: pointer to **scheduler\_ctx\***
- Failure: **NULL**

### kuaf\_ctx\_scheduler

**Function Usage<a name="section816364604611"></a>**

Acts as a unified API at the decision-making layer. It obtains information about the algorithm required by the service and invokes the configuration file module to read configuration files. Then, according to the scheduling policy, it invokes bandwidth-based scheduling or ratio-based scheduling, and records scheduling decision-making information in the **ctx** structure.

**Function Syntax<a name="section82574577467"></a>**

```c
int kuaf_ctx_scheduler(scheduler_ctx *sc_ctx)
```

**Parameters<a name="section8612145473311"></a>**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|sc_ctx|Pointer to the scheduling structure.|The structure is created by the kuaf_ctx_scheduler_create function.|Input/Output|

**Return Value<a name="section1253745513711"></a>**

- Success: **KUAF\_SUCCESS**
- Failure:
    - **KUAF\_NULL\_PTR\_ERROR**: The input **sc\_ctx** is null.
    - **KUAF\_ALG\_NOT\_SUPPORT**: The input algorithm type is not supported.
    - **KUAF\_ALGID\_ERROR**: The input algorithm ID is not supported.
    - **KUAF\_STRA\_NOT\_SUPPORT**: The input scheduling policy is not supported.

### kuaf\_ctx\_scheduler\_free

**Function Usage<a name="section816364604611"></a>**

Releases the structure resource.

**Function Syntax<a name="section82574577467"></a>**

```c
void kuaf_ctx_scheduler_free(scheduler_ctx *sc_ctx); 
```

**Parameters<a name="section8612145473311"></a>**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|sc_ctx|Pointer to the scheduling structure to be released.|The structure is created by the kuaf_ctx_scheduler_create function.|Input|

**Return Value<a name="section78901136457"></a>**

None

### kuaf\_ctx\_process\_sync

**Function Usage<a name="section816364604611"></a>**

Acts as a unified API of the execution module. It distributes execution code downwards and calls the hardware or software computing APIs of the algorithm based on information in the **ctx** structure.

**Function Syntax<a name="section82574577467"></a>**

```c
int kuaf_ctx_process_sync(scheduler_ctx *sc_ctx)
```

**Parameters<a name="section8612145473311"></a>**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|sc_ctx|Pointer to the scheduling structure.|The structure is created by the kuaf_ctx_scheduler_create function.|Input|

**Return Value<a name="section78901136457"></a>**

- Success: value returned by the hardware or software computing algorithm.
- Failure:
    - **KUAF\_NULL\_PTR\_ERROR**: The input **sc\_ctx** pointer is null.
    - **KUAF\_ALG\_NOT\_SUPPORT**: The input algorithm type is not supported.

### kuaf\_ctx\_end\_process

**Function Usage<a name="section19442111816541"></a>**

Performs post-processing after kuaf\_ctx\_process\_sync.

**Function Syntax<a name="section1144271855410"></a>**

```c
int kuaf_ctx_end_process(scheduler_ctx *sc_ctx);
```

**Parameters<a name="section1344211819546"></a>**

|Parameter|Description|Value Range|Input/Output|
|--|--|--|--|
|sc_ctx|Pointer to the scheduling structure.|The structure is created by the kuaf_ctx_scheduler_create function.|Input|

**Return Value<a name="section144437185540"></a>**

- Success: **KUAF\_SUCCESS**
- Failure:

    **KUAF\_NULL\_PTR\_ERROR**: The input structure pointer is null.

## Example

This section provides an example of using all KUAF function APIs.

1. Set environment variables.

    ```bash
    export C_INCLUDE_PATH=/usr/local/kuaf/include:$C_INCLUDE_PATH
    export LD_LIBRARY_PATH=/usr/local/kuaf/lib/:/usr/local/lib:$LD_LIBRARY_PATH
    ```

2. Create a **kuaf.c** file.
3. Press **i** to enter the insert mode and add the following content to the file:

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
        // deflateInit2_() adaptation. The ctx scheduling structure is created, and the input data of the zlib function is encapsulated into the zlib_ctx structure.
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
        // deflate adaptation
        if (kz_get_devices()) {  // Check whether KAE hardware is available.
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
        // deflateEnd adaptation
        if (kz_get_devices()) {  // Check whether KAE hardware is available.
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

4. Press **Esc**, type **:wq!**, and press **Enter** to save the file and exit.
5. Compile the **kuaf.c** file and specify the name of the output executable file as **kuaf**.

    ```bash
    gcc kuaf.c -L/usr/local/kuaf/lib -lkuaf -lkaezip -lz -lz_sw -o kuaf
    ```

6. Run the **kuaf** executable file.

    ```bash
    ./kuaf
    ```

    The execution result is as follows:

    ```text
    kuaf_ctx_scheduler_create ctx here.
    kuaf_zlib_ctx zlib_ctx malloc here.
    kuaf_ctx_shcheduler here.
    kz_deflateInit2_ call here.
    kz_deflate call here.
    kuaf_ctx_process_sync here
    ```
