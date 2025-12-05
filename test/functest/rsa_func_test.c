/*
 * Copyright (c) 2025 Huawei Technologies Co., Ltd.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 * You may obtain a copy of Mulan PSL v2 at:
 *          http://license.coscl.org.cn/MulanPSL2
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND,
 * EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT,
 * MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */
#include <stdio.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/ecdsa.h>
#include <openssl/engine.h>
#include <openssl/rsa.h>
#include <time.h>
#include <fstream>

#define ENGINE_CMD_BASE 200
#define UADK_CMD_ENABLE_RSA_ENV ENGINE_CMD_BASE
#define UADK_CMD_ENABLE_ECC_ENV (ENGINE_CMD_BASE + 1)
#define KUAF_CMD_ENABLE_ASYNC (ENGINE_CMD_BASE + 2)

#define MAX_FILE_SIZE (10 * 1024 * 1024)
#define ENCRYPTED_EXT ".rsaenc"
#define DECRYPTED_PREFIX "decrypted_"

static const unsigned char test_message[] = "This is a test message for RSA encryption and decryption";
static const size_t test_message_len = sizeof(test_message) - 1;

static void print_errors(void) {
    unsigned long err;
    while ((err = ERR_get_error())) {
        printf("ERROR: %s\n", ERR_error_string(err, NULL));
    }
}

// KAE引擎初始化
static int init_kae_engine(void) {
    printf("Initializing KAE engine...\n");
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
    ENGINE *kae_engine = ENGINE_by_id("kae");
    if (!kae_engine) {
        printf("Failed to load KAE engine\n");
        print_errors();
        return 0;
    }
    
    if (!ENGINE_init(kae_engine)) {
        printf("Failed to initialize KAE engine\n");
        print_errors();
        ENGINE_free(kae_engine);
        return 0;
    }
    
    if (!ENGINE_set_default(kae_engine, ENGINE_METHOD_ALL)) {
        printf("Failed to set KAE engine as default\n");
        print_errors();
        ENGINE_finish(kae_engine);
        ENGINE_free(kae_engine);
        return 0;
    }
    
    // 获取并检查RSA方法
    RSA_METHOD *rsa_method = (RSA_METHOD *)ENGINE_get_RSA(kae_engine);
    if (rsa_method) {
        printf("KAE RSA method is available\n");
        printf("KAE engine initialized successfully\n");
    } else {
        printf("KAE RSA method not available\n");
    }
    
    return 1;
}

static void cleanup_kae_engine(void) {
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
    ENGINE *kae_engine = ENGINE_by_id("kae");
    if (kae_engine) {
        ENGINE_finish(kae_engine);
        ENGINE_free(kae_engine);
        printf("KAE engine cleaned up\n");
    }
}

// 生成RSA密钥对
static RSA *generate_rsa_keypair(int bits) {
    printf("Generating RSA-%d keypair...\n", bits);
    
    RSA *rsa_key = RSA_new();
    if (!rsa_key) {
        print_errors();
        return NULL;
    }
    
    BIGNUM *bn = BN_new();
    if (!bn) {
        RSA_free(rsa_key);
        return NULL;
    }
    
    BN_set_word(bn, RSA_F4);
    
    if (RSA_generate_key_ex(rsa_key, bits, bn, NULL) <= 0) {
        printf("Failed to generate RSA keypair\n");
        print_errors();
        BN_free(bn);
        RSA_free(rsa_key);
        return NULL;
    }
    
    BN_free(bn);
    printf("RSA-%d keypair generated successfully\n", bits);
    return rsa_key;
}

static int test_rsa_env_control(void) {
    printf("\n=== Testing RSA Environment Control ===\n");
    
    ENGINE *kae_engine = ENGINE_by_id("kae");
    if (!kae_engine) {
        printf("KAE engine not available\n");
        return 0;
    }
    
    int ret = 1;
    
    printf("Test 1: Enabling RSA environment...\n");
    if (ENGINE_ctrl(kae_engine, UADK_CMD_ENABLE_RSA_ENV, 1, NULL, NULL) > 0) {
        printf("RSA environment enabled successfully\n");
    } else {
        printf("Failed to enable RSA environment\n");
        ret = 0;
    }
    
    RSA *test_key = generate_rsa_keypair(1024);
    if (test_key) {
        printf("RSA operations work with RSA environment enabled\n");
        RSA_free(test_key);
    } else {
        printf("RSA operations failed with RSA environment enabled\n");
        ret = 0;
    }
    
    printf("Test 2: Disabling RSA environment...\n");
    if (ENGINE_ctrl(kae_engine, UADK_CMD_ENABLE_RSA_ENV, 0, NULL, NULL) > 0) {
        printf("RSA environment disabled successfully\n");
    } else {
        printf("Failed to disable RSA environment\n");
        ret = 0;
    }
    
    ENGINE_ctrl(kae_engine, UADK_CMD_ENABLE_RSA_ENV, 1, NULL, NULL);
    
    return ret;
}

// 测试异步功能
static int test_async_functionality(void) {
    printf("\n=== Testing Async Functionality ===\n");
    
    ENGINE *kae_engine = ENGINE_by_id("kae");
    if (!kae_engine) {
        printf("KAE engine not available\n");
        return 0;
    }
    
    int ret = 1;
    
    // 测试启用异步模式
    printf("Test 1: Enabling async mode...\n");
    if (ENGINE_ctrl(kae_engine, KUAF_CMD_ENABLE_ASYNC, 1, NULL, NULL) > 0) {
        printf("Async mode enabled successfully\n");
        
        RSA *test_key = generate_rsa_keypair(1024);
        if (test_key) {
            int rsa_size = RSA_size(test_key);
            unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
            unsigned char *decrypted = (unsigned char *)OPENSSL_malloc(rsa_size);
            
            if (ciphertext && decrypted) {
                int enc_len = RSA_public_encrypt(test_message_len, test_message, 
                                               ciphertext, test_key, RSA_PKCS1_PADDING);
                if (enc_len > 0) {
                    printf("Async RSA encryption successful\n");                    
                    int dec_len = RSA_private_decrypt(enc_len, ciphertext, decrypted, 
                                                    test_key, RSA_PKCS1_PADDING);
                    if (dec_len > 0 && memcmp(decrypted, test_message, test_message_len) == 0) {
                        printf("Async RSA decryption successful and verified\n");
                    } else {
                        printf("Async RSA decryption failed\n");
                        ret = 0;
                    }
                } else {
                    printf("Async RSA encryption failed\n");
                    ret = 0;
                }
                
                OPENSSL_free(ciphertext);
                OPENSSL_free(decrypted);
            }
            
            RSA_free(test_key);
        } else {
            printf("Failed to generate test key for async test\n");
            ret = 0;
        }
    } else {
        printf("Async mode not supported or failed to enable\n");
    }
    
    printf("Test 2: Disabling async mode...\n");
    if (ENGINE_ctrl(kae_engine, KUAF_CMD_ENABLE_ASYNC, 0, NULL, NULL) > 0) {
        printf("Async mode disabled successfully\n");
    } else {
        printf("Failed to disable async mode\n");
    }
    
    return ret;
}

static int test_enhanced_rsa_mod_exp(void) {
    printf("\n=== Testing Enhanced RSA Modular Exponentiation (kuaf_rsa_mod_exp) ===\n");
    
    RSA *rsa_key = generate_rsa_keypair(1024);
    if (!rsa_key) {
        return 0;
    }
    
    int ret = 1;
    int rsa_size = RSA_size(rsa_key);
    
    BIGNUM *m = BN_new();
    BIGNUM *e = BN_new();
    BIGNUM *d = BN_new();
    BIGNUM *n = BN_new();
    BIGNUM *r = BN_new();
    BIGNUM *r2 = BN_new();
    BN_CTX *ctx = BN_CTX_new();
    
    unsigned char *plaintext = (unsigned char *)OPENSSL_malloc(rsa_size);
    unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
    unsigned char *decrypted = (unsigned char *)OPENSSL_malloc(rsa_size);
    
    if (!m || !e || !d || !n || !r || !r2 || !ctx || !plaintext || !ciphertext || !decrypted) {
        printf("Memory allocation failed\n");
        ret = 0;
    }
    
    const BIGNUM *n_const, *e_const, *d_const;
#if OPENSSL_VERSION_NUMBER < 0x10100000L
    // OpenSSL 1.0.x
    n_const = rsa_key->n;
    e_const = rsa_key->e;
    d_const = rsa_key->d;
#else
    // OpenSSL 1.1.x
    RSA_get0_key(rsa_key, &n_const, &e_const, &d_const);
#endif
    
    BN_copy(n, n_const);
    BN_copy(e, e_const);
    BN_copy(d, d_const);
    
    memset(plaintext, 0, rsa_size);
    size_t copy_len = test_message_len < (size_t)rsa_size ? test_message_len : (size_t)rsa_size;
    memcpy(plaintext, test_message, copy_len);
    BN_bin2bn(plaintext, rsa_size, m);
    
    printf("Testing RSA modular exponentiation with different approaches...\n");

    printf("Method 1: Standard RSA NO_PADDING operations...\n");
    int ciphertext_len = RSA_public_encrypt(rsa_size, plaintext, ciphertext, rsa_key, RSA_NO_PADDING);
    if (ciphertext_len <= 0) {
        printf("Standard RSA public encrypt failed\n");
        print_errors();
        ret = 0;
    } else {
        int decrypted_len = RSA_private_decrypt(ciphertext_len, ciphertext, decrypted, rsa_key, RSA_NO_PADDING);
        if (decrypted_len <= 0 || memcmp(plaintext, decrypted, rsa_size) != 0) {
            printf("Standard RSA private decrypt failed\n");
            print_errors();
            ret = 0;
        } else {
            printf("Standard RSA NO_PADDING operations successful\n");
        }
    }

    printf("Method 2: Direct BN modular exponentiation...\n");
    
    // 加密: c = m^e mod n
    if (BN_mod_exp(r, m, e, n, ctx) <= 0) {
        printf("BN_mod_exp (encryption) failed\n");
        print_errors();
        ret = 0;
    } else {
        // 解密: m = c^d mod n  
        if (BN_mod_exp(r2, r, d, n, ctx) <= 0) {
            printf("BN_mod_exp (decryption) failed\n");
            print_errors();
            ret = 0;
        } else {
            if (BN_cmp(m, r2) != 0) {
                printf("BN modular exponentiation verification failed\n");
                ret = 0;
            } else {
                printf("Direct BN modular exponentiation successful and verified\n");
            }
        }
    }
    
    printf("Method 3: Testing large number operations...\n");
    {
        BIGNUM *a = BN_new();
        BIGNUM *b = BN_new();
        BIGNUM *mod = BN_new();
        BIGNUM *result = BN_new();
        
        if (a && b && mod && result) {
            BN_rand(a, 512, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
            BN_rand(b, 512, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
            BN_rand(mod, 1024, BN_RAND_TOP_ANY, BN_RAND_BOTTOM_ANY);
            
            // 测试模乘: (a * b) mod m
            if (BN_mod_mul(result, a, b, mod, ctx) > 0) {
                printf("BN_mod_mul operation successful\n");
            } else {
                printf("BN_mod_mul operation failed\n");
            }
            
            // 测试模幂: a^b mod m
            if (BN_mod_exp(result, a, b, mod, ctx) > 0) {
                printf("BN_mod_exp with large numbers successful\n");
            } else {
                printf("BN_mod_exp with large numbers failed\n");
            }
            
            BN_free(a);
            BN_free(b);
            BN_free(mod);
            BN_free(result);
        }
    }

    if (m) BN_free(m);
    if (e) BN_free(e);
    if (d) BN_free(d);
    if (n) BN_free(n);
    if (r) BN_free(r);
    if (r2) BN_free(r2);
    if (ctx) BN_CTX_free(ctx);
    if (plaintext) OPENSSL_free(plaintext);
    if (ciphertext) OPENSSL_free(ciphertext);
    if (decrypted) OPENSSL_free(decrypted);
    RSA_free(rsa_key);
    
    return ret;
}

static int test_rsa_method_integrity(void) {
    printf("\n=== Testing RSA Method Integrity ===\n");
    
    ENGINE *kae_engine = ENGINE_by_id("kae");
    if (!kae_engine) {
        printf("KAE engine not available\n");
        return 0;
    }
    
    int ret = 1;
    RSA_METHOD *rsa_method = (RSA_METHOD *)ENGINE_get_RSA(kae_engine);
    if (!rsa_method) {
        printf("KAE RSA method not available\n");
        return 0;
    }
    
    printf("Checking RSA method availability...\n");
    printf("KAE RSA method is available and registered\n");
    
    RSA *test_key = generate_rsa_keypair(1024);
    if (test_key) {
        int rsa_size = RSA_size(test_key);
        unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
        
        if (ciphertext) {
            int enc_len = RSA_public_encrypt(test_message_len, test_message, 
                                           ciphertext, test_key, RSA_PKCS1_PADDING);
            if (enc_len > 0) {
                printf("鉁?RSA encryption through KAE method works\n");
            } else {
                printf("鉁?RSA encryption through KAE method failed\n");
                ret = 0;
            }
            OPENSSL_free(ciphertext);
        }
        
        RSA_free(test_key);
    }
    
    return ret;
}

static int test_rsa_performance_benchmark(void) {
    printf("\n=== RSA Performance Benchmark ===\n");
    
    RSA *rsa_key = generate_rsa_keypair(2048);
    if (!rsa_key) {
        return 0;
    }
    
    int rsa_size = RSA_size(rsa_key);
    int iterations = 10;
    int ret = 1;
    
    unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
    unsigned char *decrypted = (unsigned char *)OPENSSL_malloc(rsa_size);
    
    if (!ciphertext || !decrypted) {
        printf("Memory allocation failed\n");
        if (ciphertext) OPENSSL_free(ciphertext);
        if (decrypted) OPENSSL_free(decrypted);
        RSA_free(rsa_key);
        return 0;
    }
    
    printf("Running performance test with %d iterations...\n", iterations);
    
    clock_t start_enc = clock();
    for (int i = 0; i < iterations; i++) {
        if (RSA_public_encrypt(test_message_len, test_message, ciphertext, 
                              rsa_key, RSA_PKCS1_PADDING) <= 0) {
            printf("Encryption failed in performance test\n");
            ret = 0;
            break;
        }
    }
    clock_t end_enc = clock();
    double enc_duration = ((double)(end_enc - start_enc)) / CLOCKS_PER_SEC * 1000000;
    
    clock_t start_dec = clock();
    for (int i = 0; i < iterations; i++) {
        if (RSA_private_decrypt(rsa_size, ciphertext, decrypted, 
                               rsa_key, RSA_PKCS1_PADDING) <= 0) {
            printf("Decryption failed in performance test\n");
            ret = 0;
            break;
        }
    }
    clock_t end_dec = clock();
    double dec_duration = ((double)(end_dec - start_dec)) / CLOCKS_PER_SEC * 1000000;
    
    if (ret) {
        printf("Encryption performance: %.0f microseconds for %d operations\n", 
               enc_duration, iterations);
        printf("Decryption performance: %.0f microseconds for %d operations\n", 
               dec_duration, iterations);
        printf("Average per operation - Enc: %.0f 渭s, Dec: %.0f 渭s\n", 
               enc_duration / iterations, dec_duration / iterations);
    }
    
    OPENSSL_free(ciphertext);
    OPENSSL_free(decrypted);
    RSA_free(rsa_key);
    
    return ret;
}

static int test_rsa_public_encrypt(void) {
    printf("\n=== Testing RSA Public Encrypt ===\n");
    
    RSA *rsa_key = generate_rsa_keypair(2048);
    if (!rsa_key) {
        return 0;
    }
    
    int rsa_size = RSA_size(rsa_key);
    unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
    
    if (!ciphertext) {
        printf("Memory allocation failed\n");
        RSA_free(rsa_key);
        return 0;
    }
    
    int ret = 1;
    
    int ciphertext_len = RSA_public_encrypt(test_message_len, test_message, 
                                           ciphertext, rsa_key, RSA_PKCS1_PADDING);
    if (ciphertext_len <= 0) {
        printf("RSA public encrypt failed\n");
        print_errors();
        ret = 0;
    } else {
        printf("RSA public encrypt successful, ciphertext length: %d\n", ciphertext_len);
    }
    
    OPENSSL_free(ciphertext);
    RSA_free(rsa_key);
    return ret;
}

static int test_rsa_private_decrypt(void) {
    printf("\n=== Testing RSA Private Decrypt ===\n");
    
    RSA *rsa_key = generate_rsa_keypair(2048);
    if (!rsa_key) {
        return 0;
    }
    
    int rsa_size = RSA_size(rsa_key);
    unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
    unsigned char *decrypted = (unsigned char *)OPENSSL_malloc(rsa_size);
    
    if (!ciphertext || !decrypted) {
        printf("Memory allocation failed\n");
        if (ciphertext) OPENSSL_free(ciphertext);
        if (decrypted) OPENSSL_free(decrypted);
        RSA_free(rsa_key);
        return 0;
    }
    
    int ret = 1;
    
    int ciphertext_len = RSA_public_encrypt(test_message_len, test_message, 
                                           ciphertext, rsa_key, RSA_PKCS1_PADDING);
    if (ciphertext_len <= 0) {
        printf("RSA public encrypt failed for private decrypt test\n");
        ret = 0;
    } else {
        int decrypted_len = RSA_private_decrypt(ciphertext_len, ciphertext, 
                                               decrypted, rsa_key, RSA_PKCS1_PADDING);
        if (decrypted_len <= 0) {
            printf("RSA private decrypt failed\n");
            print_errors();
            ret = 0;
        } else {
            if (decrypted_len != test_message_len || 
                memcmp(decrypted, test_message, test_message_len) != 0) {
                printf("RSA private decrypt verification failed\n");
                ret = 0;
            } else {
                printf("RSA private decrypt successful and verified\n");
            }
        }
    }
    
    OPENSSL_free(ciphertext);
    OPENSSL_free(decrypted);
    RSA_free(rsa_key);
    return ret;
}

static int read_file_content(const char *filename, unsigned char **content, size_t *content_len) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        printf("Failed to open file: %s\n", filename);
        return 0;
    }
    
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size <= 0 || file_size > MAX_FILE_SIZE) {
        printf("Invalid file size: %ld bytes\n", file_size);
        fclose(file);
        return 0;
    }
    
    *content = (unsigned char *)OPENSSL_malloc(file_size);
    if (!*content) {
        printf("Memory allocation failed for file content\n");
        fclose(file);
        return 0;
    }
    
    size_t bytes_read = fread(*content, 1, file_size, file);
    fclose(file);
    
    if (bytes_read != (size_t)file_size) {
        printf("Failed to read file content\n");
        OPENSSL_free(*content);
        return 0;
    }
    
    *content_len = bytes_read;
    printf("Read file: %s (%zu bytes)\n", filename, bytes_read);
    return 1;
}

static int write_file_content(const char *filename, const unsigned char *content, size_t content_len) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        printf("Failed to create file: %s\n", filename);
        return 0;
    }
    
    size_t bytes_written = fwrite(content, 1, content_len, file);
    fclose(file);
    
    if (bytes_written != content_len) {
        printf("Failed to write file content\n");
        return 0;
    }
    
    printf("Written file: %s (%zu bytes)\n", filename, content_len);
    return 1;
}

static int create_test_file(const char *filename, const char *content) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        printf("Failed to create test file: %s\n", filename);
        return 0;
    }
    
    fprintf(file, "%s\n", content);
    fprintf(file, "================================\n");
    fprintf(file, "RSA File Encryption Test Document\n");
    fprintf(file, "Created: %ld\n", (long)time(NULL));
    fprintf(file, "Content: %s\n", content);
    fprintf(file, "This is a test file for RSA encryption/decryption\n");
    fprintf(file, "Using KAE engine for hardware acceleration\n");
    fprintf(file, "================================\n");
    
    fclose(file);
    
    FILE *size_check = fopen(filename, "rb");
    fseek(size_check, 0, SEEK_END);
    long file_size = ftell(size_check);
    fclose(size_check);
    
    printf("Created test file: %s (%ld bytes)\n", filename, file_size);
    return 1;
}

static int verify_file_content(const char *file1, const char *file2) {
    printf("Verifying file content: %s vs %s\n", file1, file2);
    
    unsigned char *content1 = NULL, *content2 = NULL;
    size_t len1 = 0, len2 = 0;
    
    if (!read_file_content(file1, &content1, &len1) || 
        !read_file_content(file2, &content2, &len2)) {
        if (content1) OPENSSL_free(content1);
        if (content2) OPENSSL_free(content2);
        return 0;
    }
    
    int result = 0;
    if (len1 == len2 && memcmp(content1, content2, len1) == 0) {
        printf("鉁?File verification successful: contents match (%zu bytes)\n", len1);
        result = 1;
    } else {
        printf("鉁?File verification failed: sizes %zu vs %zu bytes\n", len1, len2);
    }
    
    OPENSSL_free(content1);
    OPENSSL_free(content2);
    return result;
}

static int rsa_encrypt_file(RSA *rsa_key, const char *input_file, const char *output_file) {
    printf("\nEncrypting file: %s -> %s\n", input_file, output_file);
    
    unsigned char *file_content = NULL;
    size_t file_size = 0;
    
    if (!read_file_content(input_file, &file_content, &file_size)) {
        return 0;
    }
    
    int rsa_size = RSA_size(rsa_key);
    int max_chunk_size = rsa_size - 42; // For PKCS1 padding
    
    if (file_size > (size_t)max_chunk_size) {
        printf("File too large for RSA encryption (max: %d bytes)\n", max_chunk_size);
        OPENSSL_free(file_content);
        return 0;
    }
    
    unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
    if (!ciphertext) {
        printf("Memory allocation failed for ciphertext\n");
        OPENSSL_free(file_content);
        return 0;
    }
    
    int ciphertext_len = RSA_public_encrypt(file_size, file_content, ciphertext, 
                                           rsa_key, RSA_PKCS1_PADDING);
    if (ciphertext_len <= 0) {
        printf("RSA file encryption failed\n");
        print_errors();
        OPENSSL_free(file_content);
        OPENSSL_free(ciphertext);
        return 0;
    }
    
    int success = write_file_content(output_file, ciphertext, ciphertext_len);
    
    OPENSSL_free(file_content);
    OPENSSL_free(ciphertext);
    
    if (success) {
        printf("鉁?File encryption successful: %zu -> %d bytes\n", file_size, ciphertext_len);
    }
    
    return success;
}

static int rsa_decrypt_file(RSA *rsa_key, const char *input_file, const char *output_file) {
    printf("\nDecrypting file: %s -> %s\n", input_file, output_file);
    
    unsigned char *encrypted_content = NULL;
    size_t encrypted_size = 0;
    
    if (!read_file_content(input_file, &encrypted_content, &encrypted_size)) {
        return 0;
    }
    
    int rsa_size = RSA_size(rsa_key);
    if (encrypted_size != (size_t)rsa_size) {
        printf("Invalid encrypted file size: expected %d, got %zu\n", rsa_size, encrypted_size);
        OPENSSL_free(encrypted_content);
        return 0;
    }
    
    unsigned char *decrypted = (unsigned char *)OPENSSL_malloc(rsa_size);
    if (!decrypted) {
        printf("Memory allocation failed for decrypted data\n");
        OPENSSL_free(encrypted_content);
        return 0;
    }
    
    int decrypted_len = RSA_private_decrypt(encrypted_size, encrypted_content, decrypted, 
                                           rsa_key, RSA_PKCS1_PADDING);
    if (decrypted_len <= 0) {
        printf("RSA file decryption failed\n");
        print_errors();
        OPENSSL_free(encrypted_content);
        OPENSSL_free(decrypted);
        return 0;
    }
    
    int success = write_file_content(output_file, decrypted, decrypted_len);
    
    OPENSSL_free(encrypted_content);
    OPENSSL_free(decrypted);
    
    if (success) {
        printf("鉁?File decryption successful: %zu -> %d bytes\n", encrypted_size, decrypted_len);
    }
    
    return success;
}

static int rsa_encrypt_large_file(RSA *rsa_key, const char *input_file, const char *output_file) {
    printf("\nEncrypting large file (chunked): %s -> %s\n", input_file, output_file);
    
    unsigned char *file_content = NULL;
    size_t file_size = 0;
    
    if (!read_file_content(input_file, &file_content, &file_size)) {
        return 0;
    }
    
    int rsa_size = RSA_size(rsa_key);
    int max_chunk_size = rsa_size - 42; // For PKCS1 padding
    size_t total_chunks = (file_size + max_chunk_size - 1) / max_chunk_size;
    
    printf("File size: %zu bytes, Chunk size: %d bytes, Total chunks: %zu\n", 
           file_size, max_chunk_size, total_chunks);
    
    FILE *output = fopen(output_file, "wb");
    if (!output) {
        printf("Failed to create output file: %s\n", output_file);
        OPENSSL_free(file_content);
        return 0;
    }
    
    uint32_t header[2] = {(uint32_t)file_size, (uint32_t)max_chunk_size};
    fwrite(header, sizeof(header), 1, output);
    
    int success = 1;
    unsigned char *ciphertext = (unsigned char *)OPENSSL_malloc(rsa_size);
    
    for (size_t i = 0; i < total_chunks && success; i++) {
        size_t chunk_offset = i * max_chunk_size;
        size_t chunk_size = (i == total_chunks - 1) ? 
                           (file_size - chunk_offset) : max_chunk_size;
        
        int ciphertext_len = RSA_public_encrypt(chunk_size, file_content + chunk_offset, 
                                               ciphertext, rsa_key, RSA_PKCS1_PADDING);
        if (ciphertext_len <= 0) {
            printf("Chunk %zu encryption failed\n", i + 1);
            success = 0;
            break;
        }
        
        uint32_t chunk_header[2] = {(uint32_t)chunk_size, (uint32_t)ciphertext_len};
        fwrite(chunk_header, sizeof(chunk_header), 1, output);
        fwrite(ciphertext, 1, ciphertext_len, output);
        
        
    }
    
    OPENSSL_free(ciphertext);
    OPENSSL_free(file_content);
    fclose(output);
    
    if (success) {
        printf("鉁?Large file encryption successful: %zu bytes in %zu chunks\n", 
               file_size, total_chunks);
    }
    
    return success;
}

static int rsa_decrypt_large_file(RSA *rsa_key, const char *input_file, const char *output_file) {
    printf("\nDecrypting large file (chunked): %s -> %s\n", input_file, output_file);
    
    unsigned char *encrypted_content = NULL;
    size_t encrypted_size = 0;
    
    if (!read_file_content(input_file, &encrypted_content, &encrypted_size)) {
        return 0;
    }
    
    FILE *output = fopen(output_file, "wb");
    if (!output) {
        printf("Failed to create output file: %s\n", output_file);
        OPENSSL_free(encrypted_content);
        return 0;
    }
    
    int success = 1;
    size_t read_pos = 0;
    
    if (encrypted_size < sizeof(uint32_t) * 2) {
        printf("Invalid encrypted file: missing header\n");
        fclose(output);
        OPENSSL_free(encrypted_content);
        return 0;
    }
    
    uint32_t *header = (uint32_t *)encrypted_content;
    uint32_t original_file_size = header[0];
    uint32_t max_chunk_size = header[1];
    read_pos += sizeof(uint32_t) * 2;
    
    printf("Original file size: %u bytes, Chunk size: %u bytes\n", 
           original_file_size, max_chunk_size);
    
    int rsa_size = RSA_size(rsa_key);
    unsigned char *decrypted = (unsigned char *)OPENSSL_malloc(rsa_size);
    size_t total_decrypted = 0;
    size_t chunk_count = 0;
    
    while (read_pos < encrypted_size && success) {
        if (read_pos + sizeof(uint32_t) * 2 > encrypted_size) {
            printf("Invalid chunk header at position %zu\n", read_pos);
            success = 0;
            break;
        }
        
        uint32_t *chunk_header = (uint32_t *)(encrypted_content + read_pos);
        uint32_t original_chunk_size = chunk_header[0];
        uint32_t encrypted_chunk_size = chunk_header[1];
        read_pos += sizeof(uint32_t) * 2;
        
        if (read_pos + encrypted_chunk_size > encrypted_size) {
            printf("Invalid chunk data at position %zu\n", read_pos);
            success = 0;
            break;
        }
        
        int decrypted_len = RSA_private_decrypt(encrypted_chunk_size, 
                                               encrypted_content + read_pos, 
                                               decrypted, rsa_key, RSA_PKCS1_PADDING);
        if (decrypted_len <= 0 || (uint32_t)decrypted_len != original_chunk_size) {
            printf("Chunk %zu decryption failed\n", chunk_count + 1);
            success = 0;
            break;
        }
        
        fwrite(decrypted, 1, decrypted_len, output);
        total_decrypted += decrypted_len;
        read_pos += encrypted_chunk_size;
        chunk_count++;
      
    }
    
    OPENSSL_free(decrypted);
    OPENSSL_free(encrypted_content);
    fclose(output);
    
    if (success && total_decrypted == original_file_size) {
        printf("鉁?Large file decryption successful: %zu bytes in %zu chunks\n", 
               total_decrypted, chunk_count);
    } else {
        printf("鉁?Large file decryption failed: expected %u, got %zu bytes\n", 
               original_file_size, total_decrypted);
        success = 0;
    }
    
    return success;
}

static int test_rsa_file_encryption_decryption(void) {
    printf("\n=== Testing RSA File Encryption & Decryption ===\n");
    
    RSA *rsa_key = generate_rsa_keypair(2048);
    if (!rsa_key) {
        return 0;
    }
    
    int success = 1;
    
    const char *test_filename = "test_document.txt";
    const char *encrypted_filename = "test_document.txt" ENCRYPTED_EXT;
    const char *decrypted_filename = DECRYPTED_PREFIX "test_document.txt";
    
    const char *large_test_filename = "large_test_document.txt";
    const char *large_encrypted_filename = "large_test_document.txt" ENCRYPTED_EXT;
    const char *large_decrypted_filename = DECRYPTED_PREFIX "large_test_document.txt";
    
    printf("\n--- Test 1: Small File Encryption/Decryption ---\n");
    if (!create_test_file(test_filename, "Small file RSA encryption test")) {
        success = 0;
    } else if (!rsa_encrypt_file(rsa_key, test_filename, encrypted_filename)) {
        success = 0;
    } else if (!rsa_decrypt_file(rsa_key, encrypted_filename, decrypted_filename)) {
        success = 0;
    } else if (!verify_file_content(test_filename, decrypted_filename)) {
        success = 0;
    } else {
        printf("鉁?Small file encryption/decryption test passed\n");
    }
    
    printf("\n--- Test 2: Large File Chunked Encryption/Decryption ---\n");
    
    FILE *large_file = fopen(large_test_filename, "w");
    if (large_file) {
        for (int i = 0; i < 100; i++) {
            fprintf(large_file, "Line %03d: This is a large file test for RSA chunked encryption/decryption.\n", i);
            fprintf(large_file, "Additional data to make file larger... KAE engine hardware acceleration test.\n");
        }
        fclose(large_file);
        
        FILE *size_check = fopen(large_test_filename, "rb");
        fseek(size_check, 0, SEEK_END);
        long large_file_size = ftell(size_check);
        fclose(size_check);
        printf("Created large test file: %s (%ld bytes)\n", large_test_filename, large_file_size);
        
        if (!rsa_encrypt_large_file(rsa_key, large_test_filename, large_encrypted_filename)) {
            success = 0;
        } else if (!rsa_decrypt_large_file(rsa_key, large_encrypted_filename, large_decrypted_filename)) {
            success = 0;
        } else if (!verify_file_content(large_test_filename, large_decrypted_filename)) {
            success = 0;
        } else {
            printf("鉁?Large file chunked encryption/decryption test passed\n");
        }
    } else {
        printf("Failed to create large test file\n");
        success = 0;
    }
    
    RSA_free(rsa_key);
    return success;
}

int main(void) {
    printf("Starting KAE RSA Comprehensive Test\n");
    printf("====================================\n");
    if (!init_kae_engine()) {
        printf("Failed to initialize KAE engine. Exiting.\n");
        return 1;
    }
    
    int total_tests = 0;
    int passed_tests = 0;
    
    total_tests++; passed_tests += test_rsa_env_control();
    total_tests++; passed_tests += test_async_functionality();
    total_tests++; passed_tests += test_enhanced_rsa_mod_exp();
    total_tests++; passed_tests += test_rsa_method_integrity();
    total_tests++; passed_tests += test_rsa_performance_benchmark();
    total_tests++; passed_tests += test_rsa_public_encrypt();
    total_tests++; passed_tests += test_rsa_private_decrypt();
    total_tests++; passed_tests += test_rsa_file_encryption_decryption();
    
    cleanup_kae_engine();
    
    printf("\n====================================\n");
    printf("Test Results: %d/%d tests passed\n", passed_tests, total_tests);
    
    if (passed_tests == total_tests) {
        printf("鉁?All RSA functions tested successfully!\n");
        return 0;
    } else {
        printf("鉂?Some tests failed. Check the logs above.\n");
        return 1;
    }
}