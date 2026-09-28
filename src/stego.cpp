#include "stego.h"
#include "crypto_utils.h"
#include "image_gen.h"
#include <algorithm>
#include <map>
#include <random>

namespace {
    unsigned char majorityByte(const std::vector<unsigned char>& image, size_t offset,
                               int copies, size_t stride, size_t i) {
        std::map<unsigned char, int> freq;
        for (int copy = 0; copy < copies; copy++) {
            freq[image[offset + static_cast<size_t>(copy) * stride + i]]++;
        }
        unsigned char best = 0;
        int bestCount = 0;
        for (const auto& pair : freq) {
            if (pair.second > bestCount) {
                bestCount = pair.second;
                best = pair.first;
            }
        }
        return best;
    }

    size_t encLenOffset(int row, unsigned width) {
        return static_cast<size_t>(row) * 2 * width * 4;
    }

    std::vector<size_t> shuffledIndices(size_t imageSize, const std::string& key, const std::string& salt) {
        std::vector<size_t> indices;
        indices.reserve(imageSize / 4);
        for (size_t idx = DATA_EMBEDDING_START; idx < imageSize - METADATA_BOUNDARY; idx += 4) {
            indices.push_back(idx);
        }

        const unsigned seed = deriveSeedFromKey(key, salt);
        std::mt19937 g(seed);
        std::shuffle(indices.begin(), indices.end(), g);
        return indices;
    }
}

bool hasValidDimensions(const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    return width == IMAGE_WIDTH && height == IMAGE_HEIGHT &&
           image.size() == static_cast<size_t>(width) * height * 4;
}

void embedMagic(std::vector<unsigned char>& image, unsigned width, unsigned height) {
    if (!hasValidDimensions(image, width, height)) return;
    const size_t imageSize = image.size();
    const size_t stride = imageSize / MAGIC_POSITIONS;

    for (size_t p = 0; p < MAGIC_POSITIONS; p++) {
        size_t idx = (p * stride) % (imageSize - 4);
        if (idx % 4 == 0) idx += 1;
        for (int b = 0; b < 4; b++) {
            image[idx + b] = MAGIC_BYTES[b];
        }
    }
}

bool checkMagic(const std::vector<unsigned char>& image, unsigned width, unsigned height) {
    if (!hasValidDimensions(image, width, height)) return false;
    const size_t imageSize = image.size();
    const size_t stride = imageSize / MAGIC_POSITIONS;

    size_t matches = 0;
    for (size_t p = 0; p < MAGIC_POSITIONS; p++) {
        size_t idx = (p * stride) % (imageSize - 4);
        if (idx % 4 == 0) idx += 1;
        bool ok = true;
        for (int b = 0; b < 4; b++) {
            if (image[idx + b] != MAGIC_BYTES[b]) { ok = false; break; }
        }
        if (ok) matches++;
    }
    return (matches * 100 / MAGIC_POSITIONS) >= 60;
}

bool embedPayload(std::vector<unsigned char>& image, unsigned width, unsigned height,
                  const EncryptedPayload& payload, const std::string& key) {
    if (!hasValidDimensions(image, width, height)) return false;
    if (payload.encryptedData.empty() || payload.encryptedData.size() > MAX_ENCRYPTED_LEN) return false;
    if (payload.salt.size() != SALT_LENGTH || payload.iv.size() != static_cast<size_t>(NONCE_SIZE)) return false;

    const unsigned encLen = static_cast<unsigned>(payload.encryptedData.size());
    const unsigned char encLenBytes[4] = {
        static_cast<unsigned char>(encLen & 0xFF),
        static_cast<unsigned char>((encLen >> 8) & 0xFF),
        static_cast<unsigned char>((encLen >> 16) & 0xFF),
        static_cast<unsigned char>((encLen >> 24) & 0xFF)
    };
    for (int row = 0; row < METADATA_COPIES; row++) {
        const size_t offset = encLenOffset(row, width);
        for (int i = 0; i < 4; i++) {
            image[offset + i] = encLenBytes[i];
        }
    }

    for (int copy = 0; copy < METADATA_COPIES; copy++) {
        const size_t offset = SALT_OFFSET + static_cast<size_t>(copy) * METADATA_STRIDE;
        for (size_t i = 0; i < SALT_LENGTH; i++) {
            image[offset + i] = static_cast<unsigned char>(payload.salt[i]);
        }
    }

    for (int copy = 0; copy < METADATA_COPIES; copy++) {
        const size_t offset = IV_OFFSET + static_cast<size_t>(copy) * METADATA_STRIDE;
        for (int i = 0; i < NONCE_SIZE; i++) {
            image[offset + i] = payload.iv[i];
        }
    }

    for (int copy = 0; copy < METADATA_COPIES; copy++) {
        const size_t offset = HASH_OFFSET + static_cast<size_t>(copy) * METADATA_STRIDE;
        const size_t hashLen = std::min(HASH_LENGTH, payload.passwordHash.length());
        for (size_t i = 0; i < hashLen; i++) {
            image[offset + i] = static_cast<unsigned char>(payload.passwordHash[i]);
        }
    }

    const std::vector<size_t> indices = shuffledIndices(image.size(), key, payload.salt);

    const size_t encDataSize = payload.encryptedData.size();
    for (size_t copy = 0; copy < static_cast<size_t>(REDUNDANCY_FACTOR) && copy * encDataSize < indices.size(); copy++) {
        if (indices.size() <= encDataSize) break;
        size_t base = (copy * REDUNDANCY_STRIDE) % (indices.size() - encDataSize);
        for (size_t i = 0; i < encDataSize && (base + i) < indices.size(); i++) {
            size_t idx = indices[base + i];
            size_t channelOffset = copy % 3;
            image[idx + channelOffset] = payload.encryptedData[i];
        }
    }

    embedMagic(image, width, height);
    return true;
}

std::string extractSalt(const std::vector<unsigned char>& image) {
    std::string salt;
    if (image.size() < DATA_EMBEDDING_START) return salt;

    salt.reserve(SALT_LENGTH);
    for (size_t i = 0; i < SALT_LENGTH; i++) {
        salt.push_back(static_cast<char>(majorityByte(image, SALT_OFFSET, METADATA_COPIES, METADATA_STRIDE, i)));
    }
    return salt;
}

EncryptedPayload extractPayload(const std::vector<unsigned char>& image, unsigned width,
                                 unsigned height, const std::string& key) {
    EncryptedPayload result;
    if (!hasValidDimensions(image, width, height)) return result;

    std::map<unsigned, int> encLenFreq;
    for (int row = 0; row < METADATA_COPIES; row++) {
        const size_t offset = encLenOffset(row, width);
        unsigned value = 0;
        for (int i = 0; i < 4; i++) {
            value |= (static_cast<unsigned>(image[offset + i]) << static_cast<unsigned>(i * 8));
        }
        encLenFreq[value]++;
    }
    unsigned encLen = 0;
    int maxFreq = 0;
    for (const auto& pair : encLenFreq) {
        if (pair.second > maxFreq) {
            maxFreq = pair.second;
            encLen = pair.first;
        }
    }
    if (encLen < static_cast<unsigned>(NONCE_SIZE + MAC_SIZE) || encLen > MAX_EXTRACT_LEN) return result;

    result.salt = extractSalt(image);

    result.iv.resize(NONCE_SIZE);
    for (int i = 0; i < NONCE_SIZE; i++) {
        result.iv[i] = majorityByte(image, IV_OFFSET, METADATA_COPIES, METADATA_STRIDE, static_cast<size_t>(i));
    }

    const std::vector<size_t> indices = shuffledIndices(image.size(), key, result.salt);

    std::vector<std::map<unsigned char, int>> byteFrequencies(encLen);
    for (size_t copy = 0; copy < static_cast<size_t>(REDUNDANCY_FACTOR); copy++) {
        if (indices.size() <= encLen) break;
        size_t base = (copy * REDUNDANCY_STRIDE) % (indices.size() - encLen);
        size_t channelOffset = copy % 3;
        for (size_t i = 0; i < encLen && (base + i) < indices.size(); i++) {
            size_t idx = indices[base + i] + channelOffset;
            byteFrequencies[i][image[idx]]++;
        }
    }

    result.encryptedData.resize(encLen);
    for (size_t i = 0; i < encLen; i++) {
        unsigned char mostCommon = 0;
        int maxCount = 0;
        for (const auto& pair : byteFrequencies[i]) {
            if (pair.second > maxCount) {
                maxCount = pair.second;
                mostCommon = pair.first;
            }
        }
        result.encryptedData[i] = mostCommon;
    }

    result.passwordHash.reserve(HASH_LENGTH);
    for (size_t i = 0; i < HASH_LENGTH; i++) {
        result.passwordHash.push_back(static_cast<char>(majorityByte(image, HASH_OFFSET, METADATA_COPIES, METADATA_STRIDE, i)));
    }

    return result;
}
