#include "secodeFuzz.h"
#include "stdio.h"
#include "gtest/gtest.h"
#include "kuaf_sc.h"
#include "kuaf_alg.h"

long long g_test_time = 30000000;
static constexpr size_t DF_FUZZ_EXEC_SECOND = 10800;

void FuzzSchedulerVariant()
{
    int32_t alg_type = *(int32_t *)DT_SetGetNumberRange(&g_Element[0x00], 0x10, -4096, 4096);
    int32_t alg_id = *(int32_t *)DT_SetGetNumberRange(&g_Element[0x01], 0x10, -4096, 4096);
    scheduler_ctx *ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_end_process(ctx);
    kuaf_ctx_scheduler_free(ctx);
}

void FuzzSchedulerNormal()
{
    int32_t alg_type = ALG_COMP;
    int32_t alg_id = ALG_ZLIB_DEFLATE;
    scheduler_ctx *ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_end_process(ctx);
    kuaf_ctx_scheduler_free(ctx);

    alg_type = ALG_COMP;
    alg_id = ALG_ZLIB_INFLATE;
    ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_end_process(ctx);
    kuaf_ctx_scheduler_free(ctx);

    alg_type = ALG_CRYPTO;
    alg_id = ALG_RSA;
    ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_end_process(ctx);
    kuaf_ctx_scheduler_free(ctx);
}

void FuzzKuafProcessVariantDev()
{
    int32_t alg_type = ALG_COMP;
    int32_t alg_id = ALG_ZLIB_DEFLATE;
    scheduler_ctx *ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    ctx->dev_id = *(int32_t *)DT_SetGetNumberRange(&g_Element[0x00], 0x10, -4096, 4096);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_scheduler_free(ctx);

    alg_type = ALG_COMP;
    alg_id = ALG_ZLIB_INFLATE;
    ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    ctx->dev_id = *(int32_t *)DT_SetGetNumberRange(&g_Element[0x01], 0x10, -4096, 4096);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_scheduler_free(ctx);

    alg_type = ALG_CRYPTO;
    alg_id = ALG_RSA;
    ctx = kuaf_ctx_scheduler_create(alg_type, alg_id);
    kuaf_ctx_scheduler(ctx);
    ctx->dev_id = *(int32_t *)DT_SetGetNumberRange(&g_Element[0x02], 0x10, -4096, 4096);
    kuaf_ctx_process_sync(ctx);
    kuaf_ctx_scheduler_free(ctx);
}

TEST(KuafFuzz, sc_variant)
{
    DT_FUZZ_START(0, g_test_time, const_cast<char *>("sc_variant"), 0);
    DT_Enable_Leak_Check(0, 0);
    FuzzSchedulerVariant();
    DT_FUZZ_END();
}

TEST(KuafFuzz, sc_normal)
{
    DT_FUZZ_START(0, g_test_time, const_cast<char *>("sc_normal"), 0);
    DT_Enable_Leak_Check(0, 0);
    FuzzSchedulerNormal();
    DT_FUZZ_END();
}

TEST(KuafFuzz, processVariantDev)
{
    DT_FUZZ_START(0, g_test_time, const_cast<char *>("processVariantDev"), 0);
    DT_Enable_Leak_Check(0, 0);
    FuzzKuafProcessVariantDev();
    DT_FUZZ_END();
}

int main(int argc, char **argv)
{
    DT_Set_Running_Time_Second(DF_FUZZ_EXEC_SECOND);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}