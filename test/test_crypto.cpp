#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
#include "crypto_utils.h"
#include <cstring>
#include <sodium.h>

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

TEST_CASE("XChaCha20-Poly1305 encrypt then decrypt roundtrip", "[crypto]") {
    std::string key = generateRandomString(KEY_SIZE);

    SECTION("short plaintext") {
        std::string plaintext = "hello";
        std::vector<unsigned char> blob = encrypt(plaintext, key);
        REQUIRE_FALSE(blob.empty());
        REQUIRE(blob.size() >= plaintext.size() + NONCE_SIZE + MAC_SIZE);
        std::string decrypted = decrypt(blob, key);
        REQUIRE(decrypted == plaintext);
    }

    SECTION("medium plaintext") {
        std::string plaintext(100, 'A');
        std::vector<unsigned char> blob = encrypt(plaintext, key);
        REQUIRE_FALSE(blob.empty());
        std::string decrypted = decrypt(blob, key);
        REQUIRE(decrypted == plaintext);
    }

    SECTION("empty plaintext") {
        std::string plaintext;
        std::vector<unsigned char> blob = encrypt(plaintext, key);
        REQUIRE_FALSE(blob.empty());
        std::string decrypted = decrypt(blob, key);
        REQUIRE(decrypted == plaintext);
    }
}

TEST_CASE("XChaCha20-Poly1305 decryption fails with wrong key", "[crypto]") {
    std::string key = generateRandomString(KEY_SIZE);
    std::string wrongKey = generateRandomString(KEY_SIZE);

    std::string plaintext = "secret data";
    std::vector<unsigned char> blob = encrypt(plaintext, key);
    std::string decrypted = decrypt(blob, wrongKey);
    REQUIRE(decrypted.empty());
}

TEST_CASE("XChaCha20-Poly1305 decryption fails with tampered data", "[crypto]") {
    std::string key = generateRandomString(KEY_SIZE);

    std::string plaintext = "tamper test";
    std::vector<unsigned char> blob = encrypt(plaintext, key);
    REQUIRE_FALSE(blob.empty());

    if (blob.size() > static_cast<size_t>(NONCE_SIZE + 1)) {
        blob[NONCE_SIZE + 5] ^= 0x01;
    }

    std::string decrypted = decrypt(blob, key);
    REQUIRE(decrypted.empty());
}

TEST_CASE("Argon2id produces deterministic output", "[crypto]") {
    std::string password = "testpassword";
    std::string salt = "abcdefghijklmnop";
    size_t keyLen = 32;

    std::string key1 = deriveKey(password, salt, keyLen);
    std::string key2 = deriveKey(password, salt, keyLen);

    REQUIRE_FALSE(key1.empty());
    REQUIRE(key1.size() == keyLen);
    REQUIRE(key1 == key2);
}

TEST_CASE("Argon2id produces different output with different salts", "[crypto]") {
    std::string password = "testpassword";
    std::string salt1 = "abcdefghijklmnop";
    std::string salt2 = "qrstuvwxyz012345";

    std::string key1 = deriveKey(password, salt1, 32);
    std::string key2 = deriveKey(password, salt2, 32);

    REQUIRE(key1 != key2);
}

TEST_CASE("Argon2id produces different output with different passwords", "[crypto]") {
    std::string salt = "abcdefghijklmnop";

    std::string key1 = deriveKey("password1", salt, 32);
    std::string key2 = deriveKey("password2", salt, 32);

    REQUIRE(key1 != key2);
}

TEST_CASE("secureWipe zeroes memory", "[crypto]") {
    std::vector<unsigned char> data = {0x41, 0x42, 0x43, 0x44, 0x45};
    secureWipe(data.data(), data.size());

    for (size_t i = 0; i < data.size(); i++) {
        REQUIRE(data[i] == 0);
    }
}

TEST_CASE("deriveSeedFromKey is deterministic", "[crypto]") {
    std::string key = generateRandomString(KEY_SIZE);
    std::string salt = generateRandomString(16);

    unsigned seed1 = deriveSeedFromKey(key, salt);
    unsigned seed2 = deriveSeedFromKey(key, salt);

    REQUIRE(seed1 == seed2);
}

TEST_CASE("deriveSeedFromKey produces different seeds for different inputs", "[crypto]") {
    std::string key = generateRandomString(KEY_SIZE);
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
