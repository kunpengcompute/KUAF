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

#include <cstdio>
#include <iostream>
#include <cstring>
#include <string>
#include <fstream>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <vector>

using namespace std;

#define ENGINE_CMD_BASE 200
#define UADK_CMD_ENABLE_RSA_ENV ENGINE_CMD_BASE
#define UADK_CMD_ENABLE_ECC_ENV (ENGINE_CMD_BASE + 1)
#define UADK_CMD_ENABLE_ASYNC (ENGINE_CMD_BASE + 2)
#define OPENSSL_SUCCESS 1
#define OPENSSL_FAIL 0

#define ENCRYPTED_FILE_EXT ".sm2enc"
#define MAX_FILE_BUFFER_SIZE (10 * 1024 * 1024)


static inline void PrintBufferHex(const char* buffer, size_t size)
{
    for (size_t i = 0; i < size; ++i) {
        if (i % 10 == 0) {
            fprintf(stderr, "\n");
        }
        fprintf(stderr, "%#04x\t", buffer[i]);
    }
    fprintf(stderr, "\n");
}

static inline void PrintOpensslError(const char* msg)
{
    char err_buf[512];
    unsigned long err = ERR_get_error();
    if (err) {
        ERR_error_string_n(err, err_buf, sizeof(err_buf));
        cout << msg << ": " << err_buf << endl;
    } else {
        cout << msg << endl;
    }
}


string ReadFileToString(const string& filename)
{
    ifstream file(filename, ios::binary);
    if (!file) {
        throw runtime_error("Cannot open file: " + filename);
    }
    
    string content;
    file.seekg(0, ios::end);
    content.resize(file.tellg());
    file.seekg(0, ios::beg);
    file.read(&content[0], content.size());
    file.close();
    
    return content;
}


void WriteStringToFile(const string& filename, const string& content)
{
    ofstream file(filename, ios::binary);
    if (!file) {
        throw runtime_error("Cannot create file: " + filename);
    }
    file.write(content.c_str(), content.size());
    file.close();
}


bool CheckFileSize(const string& filename)
{
    ifstream file(filename, ios::binary | ios::ate);
    if (!file) {
        return false;
    }
    size_t size = file.tellg();
    file.close();
    return size <= MAX_FILE_BUFFER_SIZE;
}

EVP_PKEY* SM2_CreateEVP_PKEY(const string& keyStr, bool isPublic)
{
    BIO* bio_key = BIO_new_mem_buf(keyStr.c_str(), -1);
    EVP_PKEY* evp_pkey = isPublic ?
        PEM_read_bio_PUBKEY(bio_key, NULL, NULL, NULL) : PEM_read_bio_PrivateKey(bio_key, NULL, NULL, NULL);

    EVP_PKEY_set_alias_type(evp_pkey, EVP_PKEY_SM2);
    BIO_free_all(bio_key);
    return evp_pkey;
}

void SM2_GenKey(string& prikeyStr, string& pubkeyStr)
{
    EC_KEY *ec_key = EC_KEY_new_by_curve_name(NID_sm2);
    EC_GROUP* ec_group = EC_GROUP_new_by_curve_name(NID_sm2);
    EC_KEY_set_group(ec_key, ec_group);

    EC_KEY_generate_key(ec_key);
    BIO* bpri_key = BIO_new(BIO_s_mem());
    BIO* bpub_key = BIO_new(BIO_s_mem());
    PEM_write_bio_ECPrivateKey(bpri_key, ec_key, NULL, NULL, 0, NULL, NULL);
    PEM_write_bio_EC_PUBKEY(bpub_key, ec_key);

    size_t prk_len = BIO_pending(bpri_key);
    size_t pbk_len = BIO_pending(bpub_key);
    fprintf(stderr, "private_key_len = %lu, public_key_len = %lu\n", prk_len, pbk_len);

    char *prkBuff = new char[prk_len + 1]();
    char *pbkBuff = new char[pbk_len + 1]();
    BIO_read(bpri_key, prkBuff, prk_len);
    BIO_read(bpub_key, pbkBuff, pbk_len);
    prikeyStr = prkBuff;
    pubkeyStr = pbkBuff;

free_mem:
    delete[] pbkBuff;
    delete[] prkBuff;
    BIO_free_all(bpub_key);
    BIO_free_all(bpri_key);
    EC_GROUP_free(ec_group);
    EC_KEY_free(ec_key);
}



bool Test_SM2_Ctrl(EVP_PKEY* pkey, const string& sm2_id, ENGINE* e = NULL)
{
    cout << "\n=== Testing SM2 Control Functions (hpre_sm2_ctrl) ===" << endl;
    
    if (!pkey) {
        cout << "ERROR: Key is NULL" << endl;
        return false;
    }

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, e);
    if (!ctx) {
        PrintOpensslError("Failed to create PKEY context for control test");
        return false;
    }

    bool success = true;


    cout << "Test 1: Setting SM2 ID via ctrl..." << endl;
    if (EVP_PKEY_CTX_set1_id(ctx, (const unsigned char*)sm2_id.c_str(), sm2_id.size()) > 0) {
        cout << "SM2 ID set successfully: " << sm2_id << " (length: " << sm2_id.size() << ")" << endl;
    } else {
        PrintOpensslError("Failed to set SM2 ID");
        success = false;
    }


    cout << "Test 2: Testing SM2 ID operations (OpenSSL 1.0 compatible)..." << endl;
    

    if (EVP_PKEY_CTX_ctrl(ctx, -1, EVP_PKEY_OP_TYPE_SIG, 
                         EVP_PKEY_CTRL_MD, 0, (void*)EVP_sm3()) > 0) {
        cout << "SM2 digest type set successfully" << endl;
    } else {
        cout << "Failed to set SM2 digest type" << endl;
        success = false;
    }
    EVP_PKEY_CTX_free(ctx);
    
    if (success) {
        cout << "SM2 control functions test: SUCCESS" << endl;
    } else {
        cout << "SM2 control functions test: PARTIAL SUCCESS" << endl;
    }
    
    return success;
}


bool Test_SM2_Ctrl_Str(EVP_PKEY* pkey, ENGINE* e = NULL)
{
    cout << "\n=== Testing SM2 String Control Functions (hpre_sm2_ctrl_str) ===" << endl;
    
    if (!pkey) {
        cout << "ERROR: Key is NULL" << endl;
        return false;
    }

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, e);
    if (!ctx) {
        PrintOpensslError("Failed to create PKEY context for string control test");
        return false;
    }

    bool success = true;


    struct CtrlTest {
        const char* cmd;
        const char* value;
    };
    
   CtrlTest ctrl_tests[] = {
    {"ec_paramgen_curve", "SM2"},
    {"ec_param_enc", "named_curve"},
    {"ec_param_enc", "explicit"},
    {"ec_paramgen_curve", "prime256v1"},
    {"ec_paramgen_curve", "secp384r1"},
    {"ec_paramgen_curve", "brainpoolP256r1"},
};

    for (size_t i = 0; i < sizeof(ctrl_tests)/sizeof(ctrl_tests[0]); i++) {
    const char* cmd = ctrl_tests[i].cmd;
    const char* value = ctrl_tests[i].value;
    
    cout << "Testing control string: " << cmd << " = " << value << endl;
    
    if (EVP_PKEY_CTX_ctrl_str(ctx, cmd, value) > 0) {
        cout << "Control string '" << cmd << "' set successfully" << endl;
    } else {

        if (strcmp(cmd, "ec_paramgen_curve") == 0) {
            cout << "This curve might not be supported, continuing..." << endl;
        } else {
            success = false;
        }
    }
}

    EVP_PKEY_CTX_free(ctx);
    
    if (success) {
        cout << "SM2 string control functions test: SUCCESS" << endl;
    } else {
        cout << "SM2 string control functions test: PARTIAL SUCCESS" << endl;
    }
    
    return success;
}


string SM2_enc(EVP_PKEY* pubKey, const string& message, ENGINE* e = NULL)
{
    size_t encSize = 1024;
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(pubKey, e);
    EVP_PKEY_encrypt_init(pkey_ctx);
    if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_MD, -1, (void *)EVP_sm3()) <= 0) {
        cout << "SM2 EVP_PKEY_CTX_ctr control pctx failed" << endl;
    }

    const unsigned char* pMsg = (const unsigned char*)message.c_str();

    unsigned char* pEncMsg = new unsigned char[1024+1]();
    EVP_PKEY_encrypt(pkey_ctx, pEncMsg, &encSize, pMsg, message.size());
    cout << "enc length is " << encSize << endl;
    string retStr((const char*)pEncMsg, encSize);

free_mem:
    delete[] pEncMsg;
    EVP_PKEY_CTX_free(pkey_ctx);

    return retStr;
}

string SM2_dec(EVP_PKEY* priKey, const string& encMsg, ENGINE* e = NULL)
{
    size_t decSize = 1024;
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(priKey, e);
    EVP_PKEY_decrypt_init(pkey_ctx);

    if (EVP_PKEY_CTX_ctrl(pkey_ctx, -1, -1, EVP_PKEY_CTRL_MD, -1, (void *)EVP_sm3()) <= 0) {
        cout << "SM2 EVP_PKEY_CTX_ctr control pctx failed" << endl;
    }

    const unsigned char* pMsg = (const unsigned char*)encMsg.c_str();

    unsigned char* pDecMsg = new unsigned char[1024+1]();
    EVP_PKEY_decrypt(pkey_ctx, pDecMsg, &decSize, pMsg, encMsg.size());
    cout << "dec length is " << decSize << endl;
    string retStr((const char*)pDecMsg, decSize);

free_mem:
    delete[] pDecMsg;
    EVP_PKEY_CTX_free(pkey_ctx);

    return retStr;
}

string SM2_sign(EVP_PKEY* priKey, const string& message, const string& sm2_id, ENGINE* e = NULL)
{
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(priKey, e);
    EVP_PKEY_CTX_set1_id(pkey_ctx, sm2_id.c_str(), sm2_id.size());
    EVP_MD_CTX_set_pkey_ctx(md_ctx, pkey_ctx);

    EVP_DigestSignInit(md_ctx, NULL, EVP_sm3(), e, priKey);

    size_t sign_len = 1024;
    const unsigned char* pmsg = (const unsigned char*)message.c_str();

    cout << "sign_len = " << sign_len << endl;
    unsigned char* signBuff = new unsigned char[1024 + 1]();

    EVP_DigestSign(md_ctx, signBuff, &sign_len, pmsg, message.size());
    string retStr((const char*)signBuff, sign_len);

free_mem:
    delete[] signBuff;
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_CTX_free(pkey_ctx);

    return retStr;
}

void SM2_verify(EVP_PKEY* pubKey, const string& message, const string& signMsg, const string& sm2_id,
    ENGINE* e = NULL)
{
    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    EVP_PKEY_CTX* pkey_ctx = EVP_PKEY_CTX_new(pubKey, e);
    EVP_PKEY_CTX_set1_id(pkey_ctx, sm2_id.c_str(), sm2_id.size());
    EVP_MD_CTX_set_pkey_ctx(md_ctx, pkey_ctx);

    EVP_DigestVerifyInit(md_ctx, NULL, EVP_sm3(), e, pubKey);
    const unsigned char* pSign = (const unsigned char*)signMsg.c_str();
    const unsigned char* pmesg = (const unsigned char*)message.c_str();
    if (EVP_DigestVerify(md_ctx, pSign, signMsg.size(), pmesg, message.size()) != 1) {
        cout << "SM2 signature verify failed!" << endl;
    } else {
        cout << "SM2 signature verify succeeded!" << endl;
    }

free_mem:
    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_CTX_free(pkey_ctx);
}

bool Test_KAE_Engine_Ctrl(ENGINE* e) {
    cout << "\n=== Testing KAE Engine Control Functions ===" << endl;
    
    if (!e) {
        cout << "ERROR: Engine is NULL" << endl;
        return false;
    }


    cout << "Test 1: Enabling ECC/SM2 hardware acceleration..." << endl;
    if (ENGINE_ctrl(e, UADK_CMD_ENABLE_ECC_ENV, 1, NULL, NULL) > 0) {
        cout << "ECC/SM2 hardware acceleration enabled successfully" << endl;
    } else {
        cout << "Failed to enable ECC/SM2 hardware acceleration" << endl;
        return false;
    }


    cout << "Test 2: Enabling async mode..." << endl;
    if (ENGINE_ctrl(e, UADK_CMD_ENABLE_ASYNC, 1, NULL, NULL) > 0) {
        cout << "Async mode enabled successfully" << endl;
    } else {
        cout << "not support async mode" << endl;

    }


    cout << "Test 3: Disabling ECC/SM2 hardware acceleration..." << endl;
    if (ENGINE_ctrl(e, UADK_CMD_ENABLE_ECC_ENV, 0, NULL, NULL) > 0) {
        cout << "ECC/SM2 hardware acceleration disabled successfully" << endl;
    } else {
        cout << "Failed to disable ECC/SM2 hardware acceleration" << endl;
        return false;
    }


    ENGINE_ctrl(e, UADK_CMD_ENABLE_ECC_ENV, 1, NULL, NULL);
    
    cout << "KAE engine control test: SUCCESS" << endl;
    return true;
}


bool CreateTestFile(const string& filename, const string& content)
{
    try {
        WriteStringToFile(filename, content);
        cout << "Test file created: " << filename << " (" << content.size() << " bytes)" << endl;
        return true;
    } catch (const exception& e) {
        cout << "Error creating test file: " << e.what() << endl;
        return false;
    }
}


bool SM2_EncryptFile(EVP_PKEY* pubKey, const string& inputFile, const string& outputFile, ENGINE* e = NULL)
{
    cout << "Encrypting file: " << inputFile << " -> " << outputFile << endl;
    
    try {

        if (!CheckFileSize(inputFile)) {
            cout << "Error: File too large, maximum size is " << MAX_FILE_BUFFER_SIZE << " bytes" << endl;
            return false;
        }
        

        string fileContent = ReadFileToString(inputFile);
        cout << "File size: " << fileContent.size() << " bytes" << endl;
        

        string encryptedData = SM2_enc(pubKey, fileContent, e);
        

        WriteStringToFile(outputFile, encryptedData);
        
        cout << "File encrypted successfully. Encrypted size: " << encryptedData.size() << " bytes" << endl;
        return true;
        
    } catch (const exception& e) {
        cout << "Error during file encryption: " << e.what() << endl;
        return false;
    }
}


bool SM2_DecryptFile(EVP_PKEY* priKey, const string& inputFile, const string& outputFile, ENGINE* e = NULL)
{
    cout << "Decrypting file: " << inputFile << " -> " << outputFile << endl;
    
    try {

        if (!CheckFileSize(inputFile)) {
            cout << "Error: File too large, maximum size is " << MAX_FILE_BUFFER_SIZE << " bytes" << endl;
            return false;
        }
        

        string encryptedContent = ReadFileToString(inputFile);
        cout << "Encrypted file size: " << encryptedContent.size() << " bytes" << endl;
        

        string decryptedData = SM2_dec(priKey, encryptedContent, e);
        

        WriteStringToFile(outputFile, decryptedData);
        
        cout << "File decrypted successfully. Decrypted size: " << decryptedData.size() << " bytes" << endl;
        return true;
        
    } catch (const exception& e) {
        cout << "Error during file decryption: " << e.what() << endl;
        return false;
    }
}

int main()
{
    string prikeyStr, pubkeyStr;
    SM2_GenKey(prikeyStr, pubkeyStr);
    cout << "\n----------------------------TEST SM2---------------------------------------\n";
    cout << "prikey : \n" << prikeyStr << "\n\npubkey: \n" << pubkeyStr << endl;

    EVP_PKEY* evp_priKey = SM2_CreateEVP_PKEY(prikeyStr, false);
    EVP_PKEY* evp_pubKey = SM2_CreateEVP_PKEY(pubkeyStr, true);
    if (!evp_priKey || !evp_pubKey) {
        cout << "evp key is NULL" << endl;
        return -1;
    }

    OpenSSL_add_all_algorithms();
    ENGINE_load_dynamic();
    OPENSSL_init_crypto(OPENSSL_INIT_LOAD_CONFIG, NULL);
    ENGINE* e = ENGINE_by_id("kae");
    if (!e) {
        cout << "Engine kae failed!" << endl;
    }

    const string message("Hello openssl sm2!");
    const string sm2_id("snowdance1997");

    cout << "\n---------------------------KAE ENGINE EXTENDED TESTS---------------------------------------\n";
    /* 娴嬭瘯KAE寮曟搸鐨勬墿灞曞姛鑳?*/
    

    Test_SM2_Ctrl(evp_priKey, sm2_id, e);
    
    

    Test_SM2_Ctrl_Str(evp_priKey, e);
    



    Test_KAE_Engine_Ctrl(e);

    cout << "\n---------------------------BASIC SM2 OPERATIONS---------------------------------------\n";
    /* 鍩烘湰SM2鎿嶄綔娴嬭瘯 */
    cout << "\n---------------------------sign and verify---------------------------------------\n";
    /* 绛惧悕涓庨獙绛?*/
    string signStr = SM2_sign(evp_priKey, message, sm2_id, e);
    PrintBufferHex(signStr.c_str(), signStr.size());
    SM2_verify(evp_pubKey, message, signStr, sm2_id, e);

    cout << "\n---------------------------FILE ENCRYPTION AND DECRYPTION---------------------------------------\n";
    

    const string testFileName = "test_document.txt";
    const string encryptedFileName = testFileName + ENCRYPTED_FILE_EXT;
    const string decryptedFileName = "decrypted_" + testFileName;
    
    string testContent = "This is a test document for SM2 file encryption.\n"
                        "SM2 is a Chinese national standard for public key cryptography.\n"
                        "This file will be encrypted and then decrypted to verify the process.\n"
                        "Line 4: Test data for encryption\n"
                        "Line 5: More test content here";
    

    if (CreateTestFile(testFileName, testContent)) {

        if (SM2_EncryptFile(evp_pubKey, testFileName, encryptedFileName, e)) {

            if (SM2_DecryptFile(evp_priKey, encryptedFileName, decryptedFileName, e)) {

                try {
                    string originalContent = ReadFileToString(testFileName);
                    string decryptedContent = ReadFileToString(decryptedFileName);
                    
                    if (originalContent == decryptedContent) {
                        cout << "SUCCESS: File encryption and decryption verified! Contents match." << endl;
                    } else {
                        cout << "ERROR: Decrypted content does not match original!" << endl;
                        cout << "Original size: " << originalContent.size() << " bytes" << endl;
                        cout << "Decrypted size: " << decryptedContent.size() << " bytes" << endl;
                    }
                } catch (const exception& ex) {
                    cout << "Error verifying files: " << ex.what() << endl;
                }
            }
        }
    }

    cout << "\n---------------------------string encode and decode---------------------------------------\n";
    PrintBufferHex(message.c_str(), message.size());
    string encStr = SM2_enc(evp_pubKey, message, e);
    string decStr = SM2_dec(evp_priKey, encStr,  e);
    PrintBufferHex(decStr.c_str(), decStr.size());
    cout << "[PUBKEY]" << evp_pubKey << endl;
    cout << "[PRIKEY]" << evp_priKey << endl;
    cout << "[message]" << message << endl;

    cout << "[decStr]" << decStr << endl;

    if (message == decStr) {
        cout << "[RES]SM2 enc&dec succeeded!" << endl;
    } else {
        cout << "[RES]SM2 enc&dec failed!" << endl;
    }

    if (e) ENGINE_free(e);
    EVP_PKEY_free(evp_priKey);
    EVP_PKEY_free(evp_pubKey);

    return 0;
}