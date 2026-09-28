#include "image_utils.h"
#include "image_gen.h"
#include "stego.h"
#include "crypto_utils.h"
#include "../Include/lodepng.h"
#include <iostream>
#include <random>
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sodium.h>

namespace {
    constexpr size_t FILENAME_RANDOM_LENGTH = 10;
    constexpr size_t PNG_HEADER_SIZE = 33;

    bool hasPassPixDimensions(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) return false;

        unsigned char header[PNG_HEADER_SIZE];
        file.read(reinterpret_cast<char*>(header), PNG_HEADER_SIZE);
        if (file.gcount() != static_cast<std::streamsize>(PNG_HEADER_SIZE)) return false;

        lodepng::State state;
        unsigned w = 0, h = 0;
        if (lodepng_inspect(&w, &h, &state, header, PNG_HEADER_SIZE) != 0) return false;
        return w == IMAGE_WIDTH && h == IMAGE_HEIGHT;
    }
}

std::string encryptPassword(const std::string& masterPassphrase, const std::string& password) {
    if (password.empty() || password.size() > MAX_PASSWORD_LENGTH) {
        std::cerr << "Error: Password must be between 1 and " << MAX_PASSWORD_LENGTH << " characters" << std::endl;
        return "";
    }

    const unsigned width = IMAGE_WIDTH;
    const unsigned height = IMAGE_HEIGHT;

    std::vector<unsigned char> image;
    image.resize(static_cast<size_t>(width) * height * 4);

    std::random_device rd;
    std::mt19937 rng(rd());

    std::uniform_int_distribution<int> colorDist(0, 255);
    std::vector<unsigned char> color1 = {
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        255
    };
    std::vector<unsigned char> color2 = {
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        static_cast<unsigned char>(colorDist(rng)),
        255
    };

    generateGradient(image, width, height, color1, color2);

    std::uniform_int_distribution<int> numShapesDist(MIN_SHAPES, MAX_SHAPES);
    int numShapes = numShapesDist(rng);
    addShapes(image, width, height, numShapes, rng);
    addNaturalNoise(image, width, height, NOISE_INTENSITY);

    std::string salt = generateRandomString(SALT_LENGTH);

    std::string key = deriveKey(masterPassphrase, salt, KEY_SIZE);
    if (key.empty()) return "";
    lockMem(&key[0], key.size());

    std::vector<unsigned char> encryptedBlob = encrypt(password, key);
    if (encryptedBlob.size() < static_cast<size_t>(NONCE_SIZE + MAC_SIZE)) {
        secureWipe(&key[0], key.size());
        unlockMem(&key[0], key.size());
        return "";
    }

    std::vector<unsigned char> nonce(encryptedBlob.begin(),
                                      encryptedBlob.begin() + NONCE_SIZE);

    // The hash slots used to hold an unsalted SHA-256 of the password, which allowed
    // offline guessing without the master passphrase. Keep the layout, store noise.
    std::string filler(HASH_LENGTH, '\0');
    randombytes_buf(&filler[0], filler.size());

    EncryptedPayload payload;
    payload.encryptedData = encryptedBlob;
    payload.salt = salt;
    payload.iv = nonce;
    payload.passwordHash = filler;

    const bool embedded = embedPayload(image, width, height, payload, key);

    secureWipe(&key[0], key.size());
    unlockMem(&key[0], key.size());

    if (!embedded) {
        std::cerr << "Error: Failed to embed data into image" << std::endl;
        return "";
    }

    std::string filename = "img_" + generateRandomString(FILENAME_RANDOM_LENGTH) + ".png";
    while (std::filesystem::exists(filename)) {
        filename = "img_" + generateRandomString(FILENAME_RANDOM_LENGTH) + ".png";
    }

    unsigned error = lodepng::encode(filename, image, width, height);
    if (error) {
        std::cerr << "Error encoding image: " << lodepng_error_text(error) << std::endl;
        return "";
    }
    return filename;
}

std::string decryptPassword(const std::string& masterPassphrase, const std::string& filename) {
    std::vector<unsigned char> image;
    unsigned width, height;

    unsigned error = lodepng::decode(image, width, height, filename);
    if (error) {
        std::cerr << "Error decoding image: " << lodepng_error_text(error) << std::endl;
        return "";
    }

    if (!hasValidDimensions(image, width, height)) {
        std::cerr << "Error: Not a PassPix image (expected " << IMAGE_WIDTH << "x" << IMAGE_HEIGHT << ")" << std::endl;
        return "";
    }

    std::string salt = extractSalt(image);

    std::string key = deriveKey(masterPassphrase, salt, KEY_SIZE);
    if (key.empty()) return "";
    lockMem(&key[0], key.size());

    EncryptedPayload payload = extractPayload(image, width, height, key);

    std::string password;
    if (payload.encryptedData.empty()) {
        std::cerr << "Error: Not a valid PassPix image or the data is corrupted" << std::endl;
    } else {
        password = decrypt(payload.encryptedData, key);
    }

    secureWipe(&key[0], key.size());
    unlockMem(&key[0], key.size());
    return password;
}

std::vector<std::string> listEncFiles(const std::string& dir) {
    std::vector<std::string> files;
    namespace fs = std::filesystem;
    std::error_code ec;

    for (const auto& entry : fs::directory_iterator(dir, ec)) {
        if (ec) break;
        if (!entry.is_regular_file(ec)) continue;

        std::string filename = entry.path().filename().string();
        if (filename.length() < 5 || filename.substr(filename.length() - 4) != ".png")
            continue;

        // Cheap header check first so unrelated PNGs are never fully decoded
        const std::string path = entry.path().string();
        if (!hasPassPixDimensions(path)) continue;

        std::vector<unsigned char> img;
        unsigned w, h;
        if (lodepng::decode(img, w, h, path) != 0) continue;
        if (checkMagic(img, w, h)) {
            files.push_back(filename);
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}
