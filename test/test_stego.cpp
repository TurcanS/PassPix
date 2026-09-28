#include "catch_amalgamated.hpp"
#include "image_utils.h"
#include "image_gen.h"
#include "crypto_utils.h"
#include "lodepng.h"
#include "stego.h"
#include <algorithm>
#include <cctype>
#include <filesystem>

namespace {
    struct GeneratedImage {
        std::string filename;
        ~GeneratedImage() {
            if (!filename.empty()) std::filesystem::remove(filename);
        }
    };

    std::vector<unsigned char> loadImage(const std::string& filename, unsigned& w, unsigned& h) {
        std::vector<unsigned char> img;
        REQUIRE(lodepng::decode(img, w, h, filename) == 0);
        return img;
    }
}

TEST_CASE("Full encrypt-decrypt roundtrip", "[integration]") {
    std::string masterPass = "integration-test-master-passphrase";
    std::string password = "my-secret-password-123";

    GeneratedImage generated{encryptPassword(masterPass, password)};
    REQUIRE_FALSE(generated.filename.empty());
    REQUIRE(std::filesystem::exists(generated.filename));

    std::string decrypted = decryptPassword(masterPass, generated.filename);
    REQUIRE(decrypted == password);
}

TEST_CASE("Decrypt with wrong passphrase fails", "[integration]") {
    std::string masterPass = "correct-master-passphrase";
    std::string wrongPass = "wrong-master-passphrase";
    std::string password = "another-secret";

    GeneratedImage generated{encryptPassword(masterPass, password)};
    REQUIRE_FALSE(generated.filename.empty());

    std::string decrypted = decryptPassword(wrongPass, generated.filename);
    REQUIRE(decrypted.empty());
}

TEST_CASE("Encrypted PNG is valid and has correct dimensions", "[integration]") {
    GeneratedImage generated{encryptPassword("dimension-test-passphrase", "test-password")};
    REQUIRE_FALSE(generated.filename.empty());

    unsigned w, h;
    loadImage(generated.filename, w, h);
    REQUIRE(w == 1920);
    REQUIRE(h == 1080);
}

TEST_CASE("Generated files are named img_<random>.png", "[integration]") {
    GeneratedImage generated{encryptPassword("naming-test", "test")};
    const std::string& filename = generated.filename;
    REQUIRE(filename.size() == 18);
    REQUIRE(filename.substr(0, 4) == "img_");
    REQUIRE(filename.substr(filename.size() - 4) == ".png");
}

TEST_CASE("Magic signature is embedded and detected", "[integration]") {
    GeneratedImage generated{encryptPassword("magic-test-passphrase", "test")};
    REQUIRE_FALSE(generated.filename.empty());

    unsigned w, h;
    std::vector<unsigned char> img = loadImage(generated.filename, w, h);
    REQUIRE(checkMagic(img, w, h));
}

TEST_CASE("Non-PassPix PNG does not match magic", "[integration]") {
    std::vector<unsigned char> img = {255,255,255,255, 255,255,255,255, 255,255,255,255, 255,255,255,255};
    REQUIRE_FALSE(checkMagic(img, 2, 2));
}

TEST_CASE("Images no longer store a hash of the password", "[integration][security]") {
    const std::string password = "hash-leak-check";
    GeneratedImage generated{encryptPassword("hash-test-passphrase", password)};
    REQUIRE_FALSE(generated.filename.empty());

    unsigned w, h;
    std::vector<unsigned char> img = loadImage(generated.filename, w, h);
    const std::string leakedHash = sha256(password).substr(0, HASH_LENGTH);

    for (int copy = 0; copy < METADATA_COPIES; copy++) {
        const size_t offset = HASH_OFFSET + static_cast<size_t>(copy) * METADATA_STRIDE;
        const std::string stored(img.begin() + static_cast<long>(offset),
                                 img.begin() + static_cast<long>(offset + HASH_LENGTH));
        REQUIRE(stored != leakedHash);
    }
}

TEST_CASE("extractSalt recovers the embedded salt", "[integration]") {
    GeneratedImage generated{encryptPassword("salt-test-passphrase", "test")};
    REQUIRE_FALSE(generated.filename.empty());

    unsigned w, h;
    std::vector<unsigned char> img = loadImage(generated.filename, w, h);
    const std::string salt = extractSalt(img);
    REQUIRE(salt.size() == SALT_LENGTH);
    REQUIRE(std::all_of(salt.begin(), salt.end(), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) != 0;
    }));

    // A few corrupted copies are outvoted by the rest
    img[SALT_OFFSET] ^= 0xFF;
    img[SALT_OFFSET + METADATA_STRIDE] ^= 0xFF;
    REQUIRE(extractSalt(img) == salt);
}

TEST_CASE("Images with the wrong dimensions are rejected safely", "[integration][security]") {
    const std::string tinyName = "tiny_magic_test.png";
    const std::string shortName = "short_magic_test.png";

    std::vector<unsigned char> tiny(2 * 2 * 4, 0);
    for (int b = 0; b < 4; b++) tiny[1 + b] = MAGIC_BYTES[b];
    REQUIRE(lodepng::encode(tinyName, tiny, 2, 2) == 0);

    std::vector<unsigned char> shortImage(static_cast<size_t>(IMAGE_WIDTH) * 10 * 4, 0x41);
    REQUIRE(lodepng::encode(shortName, shortImage, IMAGE_WIDTH, 10) == 0);

    REQUIRE_FALSE(checkMagic(tiny, 2, 2));
    REQUIRE(extractPayload(shortImage, IMAGE_WIDTH, 10, std::string(KEY_SIZE, 'k')).encryptedData.empty());
    REQUIRE(decryptPassword("any-passphrase", tinyName).empty());
    REQUIRE(decryptPassword("any-passphrase", shortName).empty());

    std::filesystem::remove(tinyName);
    std::filesystem::remove(shortName);
}

TEST_CASE("Out-of-range payload length is rejected", "[integration][security]") {
    const std::string masterPass = "length-test-passphrase";
    GeneratedImage generated{encryptPassword(masterPass, "test")};
    REQUIRE_FALSE(generated.filename.empty());

    unsigned w, h;
    std::vector<unsigned char> img = loadImage(generated.filename, w, h);
    for (int row = 0; row < METADATA_COPIES; row++) {
        const size_t offset = static_cast<size_t>(row) * 2 * w * 4;
        img[offset] = 5;
        img[offset + 1] = 0;
        img[offset + 2] = 0;
        img[offset + 3] = 0;
    }
    REQUIRE(extractPayload(img, w, h, std::string(KEY_SIZE, 'k')).encryptedData.empty());

    for (int row = 0; row < METADATA_COPIES; row++) {
        const size_t offset = static_cast<size_t>(row) * 2 * w * 4;
        img[offset + 3] = 0x7F;
    }
    REQUIRE(extractPayload(img, w, h, std::string(KEY_SIZE, 'k')).encryptedData.empty());
}

TEST_CASE("Passwords longer than the limit are refused", "[integration]") {
    REQUIRE(encryptPassword("limit-test-passphrase", std::string(MAX_PASSWORD_LENGTH + 1, 'x')).empty());
    REQUIRE(encryptPassword("limit-test-passphrase", "").empty());
}

TEST_CASE("The longest allowed password survives a roundtrip", "[integration]") {
    const std::string password(MAX_PASSWORD_LENGTH, 'p');
    GeneratedImage generated{encryptPassword("long-password-passphrase", password)};
    REQUIRE_FALSE(generated.filename.empty());
    REQUIRE(decryptPassword("long-password-passphrase", generated.filename) == password);
}

TEST_CASE("Generated images are opaque", "[integration]") {
    GeneratedImage generated{encryptPassword("alpha-test-passphrase", "test")};
    REQUIRE_FALSE(generated.filename.empty());

    unsigned w, h;
    std::vector<unsigned char> img = loadImage(generated.filename, w, h);
    size_t opaque = 0;
    for (size_t i = 3; i < img.size(); i += 4) {
        if (img[i] == 255) opaque++;
    }
    // Only metadata and signature bytes may touch the alpha channel
    REQUIRE(opaque >= static_cast<size_t>(w) * h - 2000);
}

TEST_CASE("listEncFiles finds PassPix images and skips others", "[integration]") {
    GeneratedImage generated{encryptPassword("list-test-passphrase", "test")};
    REQUIRE_FALSE(generated.filename.empty());

    const std::string plainName = "plain_list_test.png";
    std::vector<unsigned char> plain(static_cast<size_t>(IMAGE_WIDTH) * IMAGE_HEIGHT * 4, 0x80);
    REQUIRE(lodepng::encode(plainName, plain, IMAGE_WIDTH, IMAGE_HEIGHT) == 0);

    const auto files = listEncFiles();
    REQUIRE(std::find(files.begin(), files.end(), generated.filename) != files.end());
    REQUIRE(std::find(files.begin(), files.end(), plainName) == files.end());
    REQUIRE(std::is_sorted(files.begin(), files.end()));

    std::filesystem::remove(plainName);
}
