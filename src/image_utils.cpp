#include "image_utils.h"
#include "image_gen.h"
#include "stego.h"
#include "crypto_utils.h"
#include "../Include/lodepng.h"
#include <iostream>
#include <random>
#include <filesystem>
#include <algorithm>

namespace {
    constexpr size_t FILENAME_RANDOM_LENGTH = 10;
    constexpr size_t SALT_LENGTH = 16;
}

void encryptPassword(const std::string& masterPassphrase, const std::string& password) {
    const unsigned width = IMAGE_WIDTH;
    const unsigned height = IMAGE_HEIGHT;

    std::vector<unsigned char> image;
    image.resize(static_cast<size_t>(width) * height * 4);

    std::random_device rd;
    std::mt19937 rng(rd());

    // Generate image
    std::uniform_int_distribution<int> colorDist(0, 255);
    std::vector<unsigned char> color1 = {
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng))
    };
    std::vector<unsigned char> color2 = {
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng))
    };

    generateGradient(image, width, height, color1, color2);

    std::uniform_int_distribution<int> numShapesDist(10, 25);
    int numShapes = numShapesDist(rng);
    addShapes(image, width, height, numShapes, rng);
    addNaturalNoise(image, width, height, 10.0f);

    // Encrypt password
    std::string salt = generateRandomString(SALT_LENGTH);
    std::string key = pbkdf2(masterPassphrase, salt, AES_KEY_SIZE, PBKDF2_ITERATIONS);

    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);

    std::string passwordHash = sha256(password);
    std::vector<unsigned char> encryptedData = aesEncrypt(password, key, iv);

    // Build payload and embed
    EncryptedPayload payload;
    payload.encryptedData = encryptedData;
    payload.salt = salt;
    payload.iv = iv;
    payload.passwordHash = passwordHash;
    payload.hmac = generateHMAC(encryptedData, key);

    embedPayload(image, width, height, payload, key);

    std::string filename = "enc_" + generateRandomString(FILENAME_RANDOM_LENGTH) + ".png";
    unsigned error = lodepng::encode(filename, image, width, height);
    if (error) {
        std::cout << "Error encoding image: " << lodepng_error_text(error) << std::endl;
    } else {
        std::cout << "Password encrypted to file: " << filename << std::endl;
    }
}

std::string decryptPassword(const std::string& masterPassphrase, const std::string& filename) {
    std::vector<unsigned char> image;
    unsigned width, height;

    unsigned error = lodepng::decode(image, width, height, filename);
    if (error) {
        std::cout << "Error decoding image: " << lodepng_error_text(error) << std::endl;
        return "";
    }

    // Peek at salt from known positions (no key needed — fixed offsets)
    std::string salt;
    salt.reserve(SALT_LENGTH);
    for (size_t i = 0; i < SALT_LENGTH; i++) {
        unsigned char s = image[SALT_OFFSET + i];
        salt.push_back(static_cast<char>(s));
    }

    // Derive key from master passphrase + extracted salt
    std::string key = pbkdf2(masterPassphrase, salt, AES_KEY_SIZE, PBKDF2_ITERATIONS);

    // Extract everything using the correct key
    EncryptedPayload payload = extractPayload(image, width, height, key);

    // Re-derive key with redundancy-recovered salt from payload for consistency
    key = pbkdf2(masterPassphrase, payload.salt, AES_KEY_SIZE, PBKDF2_ITERATIONS);

    // Verify HMAC
    bool hmacVerified = verifyHMAC(payload.encryptedData, payload.hmac, key);
    if (!hmacVerified) {
        std::cout << "Warning: HMAC verification failed. Data integrity cannot be guaranteed." << std::endl;
    } else {
        std::cout << "HMAC verification successful. Data integrity confirmed." << std::endl;
    }

    try {
        std::string password = aesDecrypt(payload.encryptedData, key, payload.iv);

        if (!password.empty()) {
            std::string passwordHash = sha256(password);
            const size_t hashLen = std::min(static_cast<size_t>(32), passwordHash.length());
            int validHashes = 0;
            for (size_t i = 0; i < hashLen; i++) {
                if (static_cast<int>(passwordHash[i]) == static_cast<int>(payload.passwordHash[i]))
                    validHashes++;
            }
            const float hashValidity = static_cast<float>(validHashes) / static_cast<float>(hashLen) * 100.0f;
            std::cout << "Password hash verification: " << hashValidity << "% valid" << std::endl;
            if (hashValidity < 30.0f) {
                std::cout << "Warning: Password verification failed. The data may be corrupted." << std::endl;
            }
        }
        return password;
    } catch (...) {
        std::cout << "Error: Exception during decryption process." << std::endl;
        return "";
    }
}

std::vector<std::string> listEncFiles() {
    std::vector<std::string> files;
    namespace fs = std::filesystem;
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(".", ec)) {
        if (ec) break;
        std::string filename = entry.path().filename().string();
        if (filename.length() > 4 &&
            filename.substr(0, 4) == "enc_" &&
            filename.substr(filename.length() - 4) == ".png") {
            files.push_back(filename);
        }
    }
    return files;
}
