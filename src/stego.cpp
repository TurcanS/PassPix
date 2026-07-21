#include "stego.h"
#include "crypto_utils.h"
#include <algorithm>
#include <map>
#include <random>

void embedPayload(std::vector<unsigned char>& image, unsigned width, unsigned height,
                  const EncryptedPayload& payload, const std::string& key) {
    const unsigned totalPixels = width * height;
    const size_t imageSize = static_cast<size_t>(totalPixels) * 4;

    // Store encryption length at three locations
    const unsigned encLen = static_cast<unsigned>(payload.encryptedData.size());
    const unsigned char encLenBytes[4] = {
        static_cast<unsigned char>(encLen & 0xFF),
        static_cast<unsigned char>((encLen >> 8) & 0xFF),
        static_cast<unsigned char>((encLen >> 16) & 0xFF),
        static_cast<unsigned char>((encLen >> 24) & 0xFF)
    };
    for (int i = 0; i < 4; i++) {
        image[i] = encLenBytes[i];
        image[static_cast<size_t>(width) * 4 - 4 + i] = encLenBytes[i];
        image[static_cast<size_t>(width) * 8 + i] = encLenBytes[i];
    }

    // Store salt at two locations
    for (size_t i = 0; i < payload.salt.length(); i++) {
        const unsigned char saltChar = static_cast<unsigned char>(payload.salt[i]);
        image[SALT_OFFSET + i] = saltChar;
        image[static_cast<size_t>(width) * 4 - SALT_OFFSET - i] = saltChar;
    }

    // Store IV at three locations
    for (int i = 0; i < NONCE_SIZE; i++) {
        const unsigned char ivByte = payload.iv[i];
        image[IV_OFFSET + i] = ivByte;
        image[static_cast<size_t>(width) * 4 - IV_OFFSET - i] = ivByte;
        image[(static_cast<size_t>(height) / 2 * width + width / 2) * 4 + i] = ivByte;
    }

    // Store password hash at two locations
    const size_t hashLen = std::min(static_cast<size_t>(32), payload.passwordHash.length());
    for (size_t i = 0; i < hashLen; i++) {
        const unsigned char hashChar = static_cast<unsigned char>(payload.passwordHash[i]);
        image[HASH_OFFSET + i] = hashChar;
        image[imageSize - HASH_OFFSET - i] = hashChar;
    }

    // Create shuffled indices for data embedding
    std::vector<size_t> indices;
    indices.reserve(totalPixels / 2);
    for (size_t idx = DATA_EMBEDDING_START; idx < imageSize - METADATA_BOUNDARY; idx += 4) {
        indices.push_back(idx);
    }

    const unsigned seed = deriveSeedFromKey(key, payload.salt);
    std::mt19937 g(seed);
    std::shuffle(indices.begin(), indices.end(), g);

    // Embed encrypted data with 2x redundancy
    const size_t indicesThird = indices.size() / 3;
    const size_t encDataSize = payload.encryptedData.size();
    for (size_t i = 0; i < encDataSize && i < indices.size(); i++) {
        const unsigned char dataByte = payload.encryptedData[i];
        image[indices[i]] = dataByte;
        if (i + indicesThird < indices.size()) {
            image[indices[i + indicesThird]] = dataByte;
        }
    }

}
EncryptedPayload extractPayload(const std::vector<unsigned char>& image, unsigned width,
                                 unsigned height, const std::string& key) {
    const unsigned totalPixels = width * height;
    const size_t imageSize = static_cast<size_t>(totalPixels) * 4;
    EncryptedPayload result;

    // Extract encryption length with majority voting
    unsigned encLen1 = 0, encLen2 = 0, encLen3 = 0;
    for (int i = 0; i < 4; i++) {
        const unsigned shift = static_cast<unsigned>(i * 8);
        encLen1 |= (static_cast<unsigned>(image[i]) << shift);
        encLen2 |= (static_cast<unsigned>(image[static_cast<size_t>(width) * 4 - 4 + i]) << shift);
        encLen3 |= (static_cast<unsigned>(image[static_cast<size_t>(width) * 8 + i]) << shift);
    }

    unsigned encLen;
    if (encLen1 == encLen2 || encLen1 == encLen3) {
        encLen = encLen1;
    } else if (encLen2 == encLen3) {
        encLen = encLen2;
    } else {
        std::vector<unsigned> lens = {encLen1, encLen2, encLen3};
        std::sort(lens.begin(), lens.end());
        encLen = lens[1];
        if (encLen > 10000) {
            encLen = std::min({encLen1, encLen2, encLen3});
            if (encLen > 10000) encLen = 1000;
        }
    }

    // Extract salt
    result.salt.reserve(16);
    for (size_t i = 0; i < 16; i++) {
        unsigned char salt1 = image[SALT_OFFSET + i];
        unsigned char salt2 = image[static_cast<size_t>(width) * 4 - SALT_OFFSET - i];
        result.salt.push_back((salt1 == salt2 || salt1 != 0) ? salt1 : salt2);
    }

    // Extract IV with triple redundancy
    result.iv.resize(NONCE_SIZE);
    for (int i = 0; i < NONCE_SIZE; i++) {
        unsigned char iv1 = image[IV_OFFSET + i];
        unsigned char iv2 = image[static_cast<size_t>(width) * 4 - IV_OFFSET - i];
        unsigned char iv3 = image[(static_cast<size_t>(height) / 2 * width + width / 2) * 4 + i];
        if (iv1 == iv2 || iv1 == iv3) {
            result.iv[i] = iv1;
        } else if (iv2 == iv3) {
            result.iv[i] = iv2;
        } else {
            result.iv[i] = (iv1 != 0) ? iv1 : ((iv2 != 0) ? iv2 : iv3);
        }
    }

    // Reconstruct shuffled indices and extract data
    std::vector<size_t> indices;
    indices.reserve(totalPixels / 2);
    for (size_t idx = DATA_EMBEDDING_START; idx < imageSize - METADATA_BOUNDARY; idx += 4) {
        indices.push_back(idx);
    }

    const unsigned seed = deriveSeedFromKey(key, result.salt);
    std::mt19937 g(seed);
    std::shuffle(indices.begin(), indices.end(), g);

    // Frequency-based extraction
    std::vector<std::map<unsigned char, int>> byteFrequencies(encLen);
    const size_t indicesThird = indices.size() / 3;
    for (size_t i = 0; i < encLen && i < indices.size(); i++) {
        byteFrequencies[i][image[indices[i]]]++;
        if (i + indicesThird < indices.size()) {
            byteFrequencies[i][image[indices[i + indicesThird]]]++;
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



    // Extract stored password hash
    result.passwordHash.reserve(32);
    for (size_t i = 0; i < 32 && i + HASH_OFFSET < imageSize; i++) {
        result.passwordHash.push_back(static_cast<char>(image[HASH_OFFSET + i]));
    }

    return result;
}
