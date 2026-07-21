#pragma once

#include <vector>
#include <string>

const size_t METADATA_BOUNDARY = 300;
const size_t DATA_EMBEDDING_START = 304;
const size_t SALT_OFFSET = 20;
const size_t IV_OFFSET = 100;
const size_t HASH_OFFSET = 200;
const size_t HMAC_OFFSET = 400;
const size_t HMAC_REAR_OFFSET = 40;

struct EncryptedPayload {
    std::vector<unsigned char> encryptedData;
    std::string salt;
    std::vector<unsigned char> iv;
    std::string passwordHash;
    std::vector<unsigned char> hmac;
};

void embedPayload(std::vector<unsigned char>& image, unsigned width, unsigned height,
                  const EncryptedPayload& payload, const std::string& key);
EncryptedPayload extractPayload(const std::vector<unsigned char>& image, unsigned width,
                                 unsigned height, const std::string& key);
