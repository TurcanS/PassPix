#pragma once

#include <string>
#include <vector>
#include <openssl/evp.h>
#include <openssl/sha.h>

const int KEY_SIZE = 32;
const int NONCE_SIZE = 24;
const int MAC_SIZE = 16;

std::string sha256(const std::string& data);

std::string deriveKey(const std::string& password, const std::string& salt, size_t keyLength);

std::vector<unsigned char> encrypt(const std::string& plaintext, const std::string& key);
std::string decrypt(const std::vector<unsigned char>& ciphertext, const std::string& key);

std::string generateRandomString(size_t length);

void secureWipe(void* data, size_t size);
void lockMem(void* data, size_t size);
void unlockMem(void* data, size_t size);

unsigned deriveSeedFromKey(const std::string& key, const std::string& salt);

void initCrypto();
