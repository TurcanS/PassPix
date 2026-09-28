#include "crypto_utils.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>
#include <sodium.h>

void initCrypto() {
    if (sodium_init() < 0) {
        std::cerr << "Fatal: libsodium initialization failed" << std::endl;
        std::exit(1);
    }
}

std::string generateRandomString(size_t length) {
    if (length == 0) return "";

    const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    std::vector<unsigned char> randBytes(length);
    randombytes_buf(randBytes.data(), length);

    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += chars[randBytes[i] % chars.size()];
    }
    return result;
}

std::string deriveKey(const std::string& password, const std::string& salt, size_t keyLength) {
    if (salt.size() < crypto_pwhash_SALTBYTES) {
        std::cerr << "Error: Salt too short for key derivation" << std::endl;
        return "";
    }

    std::vector<unsigned char> key(keyLength);

    if (crypto_pwhash(
            key.data(), keyLength,
            password.c_str(), password.length(),
            reinterpret_cast<const unsigned char*>(salt.c_str()),
            crypto_pwhash_OPSLIMIT_INTERACTIVE,
            crypto_pwhash_MEMLIMIT_INTERACTIVE,
            crypto_pwhash_ALG_ARGON2ID13) != 0) {
        std::cerr << "Error: Argon2id key derivation failed" << std::endl;
        return "";
    }

    std::string result(reinterpret_cast<char*>(key.data()), keyLength);
    secureWipe(key.data(), key.size());
    return result;
}

std::string sha256(const std::string& data) {
    unsigned char hash[crypto_hash_sha256_BYTES];
    crypto_hash_sha256(hash, reinterpret_cast<const unsigned char*>(data.data()), data.size());

    std::stringstream ss;
    for (int i = 0; i < static_cast<int>(crypto_hash_sha256_BYTES); i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(hash[i]);
    }
    return ss.str();
}

std::vector<unsigned char> encrypt(const std::string& plaintext, const std::string& key) {
    if (key.size() != crypto_aead_xchacha20poly1305_ietf_KEYBYTES) {
        std::cerr << "Error: Invalid key size" << std::endl;
        return {};
    }

    std::vector<unsigned char> nonce(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES);
    randombytes_buf(nonce.data(), nonce.size());

    std::vector<unsigned char> ciphertext(plaintext.size() + crypto_aead_xchacha20poly1305_ietf_ABYTES);
    unsigned long long ciphertextLen;

    if (crypto_aead_xchacha20poly1305_ietf_encrypt(
            ciphertext.data(), &ciphertextLen,
            reinterpret_cast<const unsigned char*>(plaintext.data()), plaintext.size(),
            nullptr, 0,
            nullptr,
            nonce.data(),
            reinterpret_cast<const unsigned char*>(key.data())) != 0) {
        std::cerr << "Error: Encryption failed" << std::endl;
        return {};
    }

    std::vector<unsigned char> result;
    result.reserve(nonce.size() + ciphertextLen);
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertextLen);

    return result;
}

std::string decrypt(const std::vector<unsigned char>& data, const std::string& key) {
    if (key.size() != crypto_aead_xchacha20poly1305_ietf_KEYBYTES) {
        std::cerr << "Error: Invalid key size" << std::endl;
        return "";
    }

    if (data.size() < static_cast<size_t>(crypto_aead_xchacha20poly1305_ietf_NPUBBYTES + crypto_aead_xchacha20poly1305_ietf_ABYTES)) {
        std::cerr << "Error: Ciphertext too short." << std::endl;
        return "";
    }

    const unsigned char* nonce = data.data();
    const unsigned char* ciphertext = data.data() + crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;
    size_t ciphertextLen = data.size() - crypto_aead_xchacha20poly1305_ietf_NPUBBYTES;

    std::vector<unsigned char> plaintext(ciphertextLen);
    unsigned long long plaintextLen;

    if (crypto_aead_xchacha20poly1305_ietf_decrypt(
            plaintext.data(), &plaintextLen,
            nullptr,
            ciphertext, ciphertextLen,
            nullptr, 0,
            nonce,
            reinterpret_cast<const unsigned char*>(key.data())) != 0) {
        std::cerr << "Error: Decryption failed, possibly due to corrupted data or incorrect key." << std::endl;
        return "";
    }

    plaintext.resize(plaintextLen);
    std::string result(plaintext.begin(), plaintext.end());
    secureWipe(plaintext.data(), plaintext.size());
    return result;
}

void secureWipe(void* data, size_t size) {
    sodium_memzero(data, size);
}

void wipeString(std::string& s) {
    if (!s.empty()) {
        secureWipe(&s[0], s.size());
    }
    s.clear();
}

void lockMem(void* data, size_t size) {
    sodium_mlock(data, size);
}

void unlockMem(void* data, size_t size) {
    sodium_munlock(data, size);
}

unsigned deriveSeedFromKey(const std::string& key, const std::string& salt) {
    unsigned char hash[crypto_hash_sha256_BYTES];

    crypto_hash_sha256_state state;
    crypto_hash_sha256_init(&state);
    crypto_hash_sha256_update(&state, reinterpret_cast<const unsigned char*>(key.data()), key.size());
    crypto_hash_sha256_update(&state, reinterpret_cast<const unsigned char*>(salt.data()), salt.size());
    crypto_hash_sha256_final(&state, hash);

    unsigned seed = 0;
    for (int i = 0; i < 4; i++) {
        seed |= (static_cast<unsigned>(hash[i]) << (i * 8));
    }

    secureWipe(hash, sizeof(hash));
    return seed;
}
