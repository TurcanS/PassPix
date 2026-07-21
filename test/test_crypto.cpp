#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "crypto_utils.h"
#include <cstring>

TEST_CASE("SHA-256 produces correct hash for known input", "[crypto]") {
    SECTION("empty string") {
        std::string result = sha256("");
        REQUIRE(result == "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    }
    SECTION("alphabet string") {
        std::string result = sha256("abc");
        REQUIRE(result == "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    }
}

TEST_CASE("AES-256-CBC encrypt then decrypt roundtrip", "[crypto]") {
    std::string key = generateRandomString(AES_KEY_SIZE);
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);

    SECTION("short plaintext") {
        std::string plaintext = "hello";
        std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
        REQUIRE_FALSE(ciphertext.empty());
        std::string decrypted = aesDecrypt(ciphertext, key, iv);
        REQUIRE(decrypted == plaintext);
    }

    SECTION("exact block size plaintext") {
        std::string plaintext(16, 'A');
        std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
        REQUIRE_FALSE(ciphertext.empty());
        std::string decrypted = aesDecrypt(ciphertext, key, iv);
        REQUIRE(decrypted == plaintext);
    }

    SECTION("multi-block plaintext") {
        std::string plaintext(47, 'B');
        std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
        REQUIRE_FALSE(ciphertext.empty());
        REQUIRE(ciphertext.size() >= plaintext.size());
        std::string decrypted = aesDecrypt(ciphertext, key, iv);
        REQUIRE(decrypted == plaintext);
    }
}

TEST_CASE("AES decryption with wrong key fails", "[crypto]") {
    std::string key = generateRandomString(AES_KEY_SIZE);
    std::string wrongKey = generateRandomString(AES_KEY_SIZE);
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);

    std::string plaintext = "secret data";
    std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
    std::string decrypted = aesDecrypt(ciphertext, wrongKey, iv);
    REQUIRE(decrypted != plaintext);
}

TEST_CASE("AES decryption with wrong IV fails", "[crypto]") {
    std::string key = generateRandomString(AES_KEY_SIZE);
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    std::vector<unsigned char> wrongIv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);
    RAND_bytes(wrongIv.data(), AES_BLOCK_SIZE);

    std::string plaintext = "secret data";
    std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
    std::string decrypted = aesDecrypt(ciphertext, key, wrongIv);
    REQUIRE(decrypted != plaintext);
}

TEST_CASE("PBKDF2 produces deterministic output", "[crypto]") {
    std::string password = "testpassword";
    std::string salt = "abcdefghijklmnop";
    size_t keyLen = 32;

    std::string key1 = pbkdf2(password, salt, keyLen, PBKDF2_ITERATIONS);
    std::string key2 = pbkdf2(password, salt, keyLen, PBKDF2_ITERATIONS);

    REQUIRE_FALSE(key1.empty());
    REQUIRE(key1.size() == keyLen);
    REQUIRE(key1 == key2);
}

TEST_CASE("PBKDF2 produces different output with different salts", "[crypto]") {
    std::string password = "testpassword";
    std::string salt1 = "abcdefghijklmnop";
    std::string salt2 = "qrstuvwxyz012345";

    std::string key1 = pbkdf2(password, salt1, 32, PBKDF2_ITERATIONS);
    std::string key2 = pbkdf2(password, salt2, 32, PBKDF2_ITERATIONS);

    REQUIRE(key1 != key2);
}

TEST_CASE("HMAC generation and verification", "[crypto]") {
    std::string key = generateRandomString(32);
    std::vector<unsigned char> data = {'H', 'e', 'l', 'l', 'o'};

    std::vector<unsigned char> hmac = generateHMAC(data, key);
    REQUIRE(hmac.size() == HMAC_SIZE);
    REQUIRE(verifyHMAC(data, hmac, key));
}

TEST_CASE("HMAC verification fails with wrong key", "[crypto]") {
    std::string key1 = generateRandomString(32);
    std::string key2 = generateRandomString(32);
    while (key1 == key2) key2 = generateRandomString(32);

    std::vector<unsigned char> data = {'t', 'e', 's', 't'};
    std::vector<unsigned char> hmac = generateHMAC(data, key1);

    REQUIRE_FALSE(verifyHMAC(data, hmac, key2));
}

TEST_CASE("HMAC verification fails with tampered data", "[crypto]") {
    std::string key = generateRandomString(32);
    std::vector<unsigned char> data = {'o', 'r', 'i', 'g', 'i', 'n', 'a', 'l'};
    std::vector<unsigned char> hmac = generateHMAC(data, key);

    std::vector<unsigned char> tampered = {'t', 'a', 'm', 'p', 'e', 'r', 'e', 'd'};
    REQUIRE_FALSE(verifyHMAC(tampered, hmac, key));
}

TEST_CASE("deriveSeedFromKey is deterministic", "[crypto]") {
    std::string key = generateRandomString(32);
    std::string salt = generateRandomString(16);

    unsigned seed1 = deriveSeedFromKey(key, salt);
    unsigned seed2 = deriveSeedFromKey(key, salt);

    REQUIRE(seed1 == seed2);
}

TEST_CASE("deriveSeedFromKey produces different seeds for different inputs", "[crypto]") {
    std::string key = generateRandomString(32);
    std::string salt1 = generateRandomString(16);
    std::string salt2 = generateRandomString(16);

    unsigned seed1 = deriveSeedFromKey(key, salt1);
    unsigned seed2 = deriveSeedFromKey(key, salt2);

    REQUIRE(seed1 != seed2);
}

TEST_CASE("generateRandomString produces correct length", "[crypto]") {
    std::string r1 = generateRandomString(16);
    REQUIRE(r1.size() == 16);

    std::string r2 = generateRandomString(32);
    REQUIRE(r2.size() == 32);

    std::string r3 = generateRandomString(0);
    REQUIRE(r3.empty());
}
