#include "crypto_utils.h"
#include <iostream>
#include <random>
#include <iomanip>
#include <sstream>
#include <algorithm>
#include <cstring>



std::string generateRandomString(size_t length) {
    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<> distribution(0, chars.size() - 1);
    
    std::string result;
    result.reserve(length);
    
    for (size_t i = 0; i < length; ++i) {
        result += chars[distribution(generator)];
    }
    
    return result;
}

std::string pbkdf2(const std::string& password, const std::string& salt, size_t keyLength, int iterations) {
    std::vector<unsigned char> key(keyLength);
    
    // Use PKCS5_PBKDF2_HMAC with SHA-256
    if (PKCS5_PBKDF2_HMAC(
            password.c_str(), password.length(),
            reinterpret_cast<const unsigned char*>(salt.c_str()), salt.length(),
            iterations,
            EVP_sha256(),
            keyLength, key.data()) != 1) {
        std::cerr << "Error: PBKDF2 key derivation failed" << std::endl;
        return "";
    }
    
    // Convert to std::string for our API
    std::string result(reinterpret_cast<char*>(key.data()), keyLength);
    return result;
}

std::string sha256(const std::string& data) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    const EVP_MD* md = EVP_sha256();
    
    EVP_DigestInit_ex(context, md, nullptr);
    EVP_DigestUpdate(context, data.c_str(), data.size());
    EVP_DigestFinal_ex(context, hash, nullptr);
    
    EVP_MD_CTX_free(context);
    
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::vector<unsigned char> aesEncrypt(const std::string& plaintext, const std::string& key, std::vector<unsigned char>& iv) {
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;
    
    std::vector<unsigned char> ciphertext(plaintext.length() + AES_BLOCK_SIZE);
    
    ctx = EVP_CIPHER_CTX_new();
    
    EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, 
                      reinterpret_cast<const unsigned char*>(key.c_str()), 
                      iv.data());
    
    // Explicitly set padding mode (PKCS7 padding is the default)
    EVP_CIPHER_CTX_set_padding(ctx, 1);
    
    EVP_EncryptUpdate(ctx, ciphertext.data(), &len, 
                     reinterpret_cast<const unsigned char*>(plaintext.c_str()), 
                     plaintext.length());
    ciphertext_len = len;
    
    EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len);
    ciphertext_len += len;
    
    EVP_CIPHER_CTX_free(ctx);
    
    ciphertext.resize(ciphertext_len);
    
    return ciphertext;
}

std::string aesDecrypt(const std::vector<unsigned char>& ciphertext, const std::string& key, const std::vector<unsigned char>& iv) {
    if (ciphertext.empty()) {
        std::cout << "Error: Empty ciphertext." << std::endl;
        return "";
    }
    
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    
    std::vector<unsigned char> plaintext_buf(ciphertext.size() + AES_BLOCK_SIZE);
    
    ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        std::cout << "Error: Failed to create cipher context." << std::endl;
        return "";
    }
    
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, 
                         reinterpret_cast<const unsigned char*>(key.c_str()), 
                         iv.data()) != 1) {
        std::cout << "Error: Failed to initialize decryption." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    
    // Explicitly set padding mode (PKCS7 padding is the default)
    EVP_CIPHER_CTX_set_padding(ctx, 1);
    
    if (EVP_DecryptUpdate(ctx, plaintext_buf.data(), &len, 
                       ciphertext.data(), ciphertext.size()) != 1) {
        std::cout << "Error: Failed during decryption update." << std::endl;
        EVP_CIPHER_CTX_free(ctx);
        return "";
    }
    
    plaintext_len = len;
    
    int finalResult = EVP_DecryptFinal_ex(ctx, plaintext_buf.data() + len, &len);
    
    EVP_CIPHER_CTX_free(ctx);
    
    if (finalResult <= 0) {
        std::cout << "Error: Decryption failed, possibly due to corrupted data or incorrect key/IV." << std::endl;
        return "";
    }
    
    plaintext_len += len;
    
    // Check if plaintext contains only valid characters
    bool isPrintable = true;
    for (int i = 0; i < plaintext_len; i++) {
        if (!isprint(plaintext_buf[i]) && !isspace(plaintext_buf[i])) {
            isPrintable = false;
            break;
        }
    }
    
    if (!isPrintable) {
        std::cout << "Warning: Decrypted data contains non-printable characters, which may indicate corruption." << std::endl;
    }
    
    // The padding is automatically removed by EVP_DecryptFinal_ex, so we don't need to handle null bytes
    return std::string(plaintext_buf.begin(), plaintext_buf.begin() + plaintext_len);
}

std::vector<unsigned char> generateHMAC(const std::vector<unsigned char>& data, const std::string& key) {
    std::vector<unsigned char> hmac(HMAC_SIZE);
    unsigned int len = 0;
    
    // Use proper HMAC function instead of manual concatenation
    unsigned char* result = HMAC(EVP_sha256(), 
                                  key.c_str(), 
                                  key.length(),
                                  data.data(), 
                                  data.size(),
                                  hmac.data(), 
                                  &len);
    
    if (result == nullptr) {
        std::cerr << "Error: HMAC generation failed" << std::endl;
        return std::vector<unsigned char>(HMAC_SIZE, 0);
    }
    
    hmac.resize(len);
    return hmac;
}

bool verifyHMAC(const std::vector<unsigned char>& data, const std::vector<unsigned char>& hmac, const std::string& key) {
    std::vector<unsigned char> computedHmac = generateHMAC(data, key);
    
    if (hmac.size() != computedHmac.size()) {
        return false;
    }
    
    // Use OpenSSL's constant-time comparison
    return CRYPTO_memcmp(hmac.data(), computedHmac.data(), hmac.size()) == 0;
}

unsigned deriveSeedFromKey(const std::string& key, const std::string& salt) {
    // Use SHA-256 to create a hash of key and salt
    unsigned char hash[SHA256_DIGEST_LENGTH];
    
    EVP_MD_CTX* context = EVP_MD_CTX_new();
    if (context == nullptr) {
        std::cerr << "Error: Failed to create EVP_MD_CTX" << std::endl;
        return 0;
    }
    
    const EVP_MD* md = EVP_sha256();
    
    if (EVP_DigestInit_ex(context, md, nullptr) != 1) {
        std::cerr << "Error: Failed to initialize digest" << std::endl;
        EVP_MD_CTX_free(context);
        return 0;
    }
    
    if (EVP_DigestUpdate(context, key.data(), key.size()) != 1) {
        std::cerr << "Error: Failed to update digest with key" << std::endl;
        EVP_MD_CTX_free(context);
        return 0;
    }
    
    if (EVP_DigestUpdate(context, salt.data(), salt.size()) != 1) {
        std::cerr << "Error: Failed to update digest with salt" << std::endl;
        EVP_MD_CTX_free(context);
        return 0;
    }
    
    if (EVP_DigestFinal_ex(context, hash, nullptr) != 1) {
        std::cerr << "Error: Failed to finalize digest" << std::endl;
        EVP_MD_CTX_free(context);
        return 0;
    }
    
    EVP_MD_CTX_free(context);
    
    // Convert first 4 bytes of hash to unsigned int
    // Using bit shifting for endian-independent conversion
    unsigned seed = 0;
    for (int i = 0; i < 4; i++) {
        seed |= (static_cast<unsigned>(hash[i]) << (i * 8));
    }
    
    return seed;
}
