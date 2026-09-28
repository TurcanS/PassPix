#pragma once

#include <vector>
#include <string>

const size_t METADATA_BOUNDARY = 300;
const size_t DATA_EMBEDDING_START = 1500;
const size_t SALT_OFFSET = 20;
const size_t IV_OFFSET = 500;
const size_t HASH_OFFSET = 1000;

const size_t SALT_LENGTH = 16;
const size_t HASH_LENGTH = 32;
const int METADATA_COPIES = 8;
const size_t METADATA_STRIDE = 64;

const unsigned char MAGIC_BYTES[4] = {0x50, 0x61, 0x73, 0x73};
const size_t MAGIC_POSITIONS = 512;
const int REDUNDANCY_FACTOR = 16;
const size_t REDUNDANCY_STRIDE = 1024;

// Copies k and k + 3 share a color channel and start 3 * REDUNDANCY_STRIDE apart,
// so longer payloads would overwrite each other.
const size_t MAX_ENCRYPTED_LEN = 3 * REDUNDANCY_STRIDE;
// Upper bound accepted when reading, matching what older versions could have written.
const size_t MAX_EXTRACT_LEN = 10000;

struct EncryptedPayload {
    std::vector<unsigned char> encryptedData;
    std::string salt;
    std::vector<unsigned char> iv;
    // No longer carries a password hash: new images fill these bytes with random data.
    std::string passwordHash;
};

bool hasValidDimensions(const std::vector<unsigned char>& image, unsigned width, unsigned height);

void embedMagic(std::vector<unsigned char>& image, unsigned width, unsigned height);
bool checkMagic(const std::vector<unsigned char>& image, unsigned width, unsigned height);

bool embedPayload(std::vector<unsigned char>& image, unsigned width, unsigned height,
                  const EncryptedPayload& payload, const std::string& key);
std::string extractSalt(const std::vector<unsigned char>& image);
EncryptedPayload extractPayload(const std::vector<unsigned char>& image, unsigned width,
                                 unsigned height, const std::string& key);
