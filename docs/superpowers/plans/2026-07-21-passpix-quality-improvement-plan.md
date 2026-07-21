# PassPix Quality Improvement — Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Harden PassPix for production — add tests, CI, fix docs/license, and refactor for code quality. No new features, no GUI work.

**Architecture:** Phase 1 fixes the foundation (license, README, C++17, CI). Phase 2 adds a Catch2 test suite (unit + integration). Phase 3 refactors the codebase (split image_utils, remove `using namespace std`, replace dirent.h) with tests as a safety net. The existing `encryptPassword`/`decryptPassword` API in `image_utils.h` is preserved as a compatibility wrapper during the split.

**Tech Stack:** C++17, OpenSSL (libssl, libcrypto), LodePNG (bundled), Catch2 v3 (vendored amalgamated), GNU Make + CMake 3.10+, GitHub Actions

## Global Constraints

- C++ standard: C++17 (upgraded from C++11)
- Minimum compiler: g++/MinGW 8+
- License: MIT
- No new features, no GUI work
- Image dimensions: 1920×1080
- All tests must pass with `make test` on both Linux and Windows MinGW
- Zero `using namespace std` in any source file
- Zero compiler warnings with `-Wall -Wextra -pedantic`

---

### Task 1: C++17 standard bump + build system doc fixes

**Files:**
- Modify: `Makefile:44`
- Modify: `CMakeLists.txt:4`
- Modify: `Makefile:1,114`

**Interfaces:**
- Produces: C++17 compilation across both build systems

- [ ] **Step 1: Update Makefile C++ standard**

Change `Makefile` line 44 from:
```
CXXFLAGS = -std=c++11 -Wall -Wextra -pedantic
```
to:
```
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic
```

- [ ] **Step 2: Update Makefile comments**

Line 1: change `# Makefile for Password_To_Image` to `# Makefile for PassPix`

Line 114: change `@echo "Password_To_Image Makefile"` to `@echo "PassPix Makefile"`

- [ ] **Step 3: Update CMakeLists.txt C++ standard**

Change `CMakeLists.txt` line 4 from:
```
set(CMAKE_CXX_STANDARD 11)
```
to:
```
set(CMAKE_CXX_STANDARD 17)
```

- [ ] **Step 4: Verify both build systems work**

```bash
make clean && make
```

Expected: compiles without errors. Run `./passpix` — should show menu.

```bash
mkdir -p build/cmake-test && cd build/cmake-test && cmake ../.. && make
```

Expected: compiles without errors. `./passpix` works.

- [ ] **Step 5: Commit**

```bash
git add Makefile CMakeLists.txt
git commit -m "build: bump to C++17, fix Makefile references to PassPix"
```

---

### Task 2: MIT License + README updates

**Files:**
- Create: `LICENSE`
- Modify: `README.md`

**Interfaces:**
- Produces: MIT-licensed project with accurate documentation

- [ ] **Step 1: Create MIT LICENSE file**

```bash
cat > LICENSE << 'EOF'
MIT License

Copyright (c) 2025-2026 PassPix Contributors

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
EOF
```

- [ ] **Step 2: Update README — image size references**

Change line 16 from `random 720×720 gradient image` to `random 1920×1080 gradient image`

Change line 84 from `720×720 RGBA (8-bit per channel)` to `1920×1080 RGBA (8-bit per channel)`

- [ ] **Step 3: Update README — license section**

Replace line 120: `Apache 2.0 — see [LICENSE](LICENSE).` with `MIT — see [LICENSE](LICENSE).`

- [ ] **Step 4: Update README — project structure section**

Replace lines 94-106 with:
```
PassPix/
├── src/
│   ├── main.cpp            # CLI menu and user flow
│   ├── crypto_utils.h/.cpp  # PBKDF2, AES-256-CBC, HMAC, SHA-256
│   ├── image_gen.h/.cpp     # Gradient, shapes, noise generation
│   ├── stego.h/.cpp         # Steganographic embed/extract with redundancy
│   └── image_utils.h/.cpp   # Legacy compatibility wrapper
├── Include/
│   ├── lodepng.cpp          # PNG encoding/decoding (bundled)
│   └── lodepng.h
├── test/
│   ├── test_crypto.cpp      # Unit tests for crypto functions
│   ├── test_stego.cpp       # Integration tests for encrypt/decrypt roundtrip
│   └── smoke_test.sh        # CLI smoke test
├── Makefile                 # Make build
├── CMakeLists.txt           # CMake build
├── LICENSE                  # MIT license
└── README.md
```

- [ ] **Step 5: Update README — add testing section**

After the "Usage" section (before "Technical Details"), add:

```
### Testing

```bash
# Run all tests (unit + integration + smoke)
make test

# Or with CMake
cd build && cmake .. && make && ctest
```

- [ ] **Step 6: Commit**

```bash
git add LICENSE README.md
git commit -m "docs: switch to MIT license, update README for 1920x1080, add testing section"
```

---

### Task 3: CI Pipeline (GitHub Actions)

**Files:**
- Create: `.github/workflows/ci.yml`

**Interfaces:**
- Produces: CI that builds on Linux (g++) and Windows (MinGW) and runs `make test`

- [ ] **Step 1: Create CI workflow**

```bash
mkdir -p .github/workflows
```

Write `.github/workflows/ci.yml`:

```yaml
name: CI

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]

jobs:
  linux:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install dependencies
        run: sudo apt-get update && sudo apt-get install -y build-essential libssl-dev
      - name: Build
        run: make
      - name: Test
        run: make test

  windows:
    runs-on: windows-latest
    defaults:
      run:
        shell: msys2 {0}
    steps:
      - uses: actions/checkout@v4
      - uses: msys2/setup-msys2@v2
        with:
          msystem: ucrt64
          update: true
          install: >-
            base-devel
            mingw-w64-ucrt-x86_64-toolchain
            mingw-w64-ucrt-x86_64-openssl
      - name: Build
        run: make
      - name: Test
        run: make test
```

- [ ] **Step 2: Commit**

```bash
git add .github/
git commit -m "ci: add GitHub Actions workflow for Linux and Windows builds"
```

---

### Task 4: Vend Catch2 + test infrastructure

**Files:**
- Create: `test/catch_amalgamated.hpp`
- Create: `test/catch_amalgamated.cpp`
- Create: `test/Makefile`
- Modify: `Makefile`
- Modify: `CMakeLists.txt`
- Modify: `.gitignore`

**Interfaces:**
- Produces: `make test` target that compiles and runs all tests in `test/`
- Consumes: C++17 (from Task 1)

- [ ] **Step 1: Download Catch2 v3 amalgamated files**

```bash
mkdir -p test
curl -sL https://github.com/catchorg/Catch2/releases/download/v3.8.0/catch_amalgamated.hpp -o test/catch_amalgamated.hpp
curl -sL https://github.com/catchorg/Catch2/releases/download/v3.8.0/catch_amalgamated.cpp -o test/catch_amalgamated.cpp
```

- [ ] **Step 2: Create test Makefile**

Write `test/Makefile`:

```makefile
CXX ?= g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -I../Include -I../src
LIBS = -lssl -lcrypto -lpthread

TEST_SOURCES = test_crypto.cpp test_stego.cpp
SRC_SOURCES = ../src/crypto_utils.cpp ../src/image_utils.cpp ../Include/lodepng.cpp
CATCH_SRC = catch_amalgamated.cpp

test_runner: $(TEST_SOURCES) $(SRC_SOURCES) $(CATCH_SRC)
	$(CXX) $(CXXFLAGS) -o test_runner \
		$(TEST_SOURCES) $(SRC_SOURCES) $(CATCH_SRC) $(LIBS)

.PHONY: test clean
test: test_runner
	./test_runner

clean:
	rm -f test_runner *.o
```

- [ ] **Step 3: Add `make test` to root Makefile**

Add after the `help:` target (before `# Dependencies` line at line 127) in `Makefile`:

```makefile
# Test target
.PHONY: test
test: $(TARGET)
	@echo "Running tests..."
	@$(MAKE) -C test test
```

- [ ] **Step 4: Update CMakeLists.txt for tests**

Add after the `add_executable` block (after line 73) in `CMakeLists.txt`:

```cmake
# Testing
enable_testing()

set(TEST_SOURCES
    test/catch_amalgamated.cpp
    test/test_crypto.cpp
    test/test_stego.cpp
    src/crypto_utils.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)

add_executable(test_runner ${TEST_SOURCES})
target_include_directories(test_runner PRIVATE src Include)

if(TARGET OpenSSL::SSL AND TARGET OpenSSL::Crypto)
    target_link_libraries(test_runner OpenSSL::SSL OpenSSL::Crypto)
else()
    target_include_directories(test_runner PRIVATE ${OPENSSL_INCLUDE_DIR})
    target_link_libraries(test_runner PRIVATE ${OPENSSL_SSL_LIBRARY} ${OPENSSL_CRYPTO_LIBRARY})
endif()

add_test(NAME passpix_tests COMMAND test_runner WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
```

- [ ] **Step 5: Update .gitignore**

Add to `.gitignore`:
```
# Test artifacts
test/test_runner
test/*.o
*.png
```

- [ ] **Step 6: Verify test infrastructure compiles (even with empty test files)**

Create placeholder test files:

`test/test_crypto.cpp`:
```cpp
#define CATCH_CONFIG_MAIN
#include "catch_amalgamated.hpp"
```

`test/test_stego.cpp`:
```cpp
#include "catch_amalgamated.hpp"
```

Run:
```bash
make test
```

Expected: test_runner compiles and reports "All tests passed" (0 tests).

- [ ] **Step 7: Commit**

```bash
git add test/ Makefile CMakeLists.txt .gitignore
git commit -m "test: add Catch2 v3 amalgamated + test infrastructure"
```

---

### Task 5: Remove `using namespace std`

**Files:**
- Modify: `src/main.cpp`
- Modify: `src/crypto_utils.cpp`
- Modify: `src/image_utils.cpp`

**Interfaces:**
- No API change — purely internal
- Produces: Zero `using namespace std` in any source file

- [ ] **Step 1: Fix `src/main.cpp`**

Delete line 8 (`using namespace std;`). Replace unqualified names:

- `cout` → `std::cout`
- `cin` → `std::cin`
- `string` → `std::string` (already in includes, but `main()` uses `string` on lines 31, 33, 83, 91)
- `numeric_limits<streamsize>` → `std::numeric_limits<std::streamsize>`
- `auto` → `auto` (no change needed)
- `endl` → `std::endl`

Full replacements (6 occurrences in main.cpp):
- Line 13: `cout` → `std::cout`
- Line 14: `cout << string(55, '=')` → `std::cout << std::string(55, '=')`
- Line 17: (no change, just uses `showMenu()`)
- Line 20: `cin` → `std::cin`
- Line 23-25: `cin.clear()`, `cin.ignore(numeric_limits<streamsize>::max()`, `cout` → add `std::`
- Line 28: `cin.ignore()` → `std::cin.ignore()`
- Lines 31, 33, 35, 36, 38-52, 56, 58-68, 70-74, 76-80, 83, 86-94, 97, 100-101: replace all `cout`, `cin`, `string`, `endl` with `std::` prefix
- Line 83: `string masterPassphrase` → `std::string masterPassphrase`
- Line 87: `string decrypted` → `std::string decrypted`
- Line 107: `showMenu` function — replace `cout`, `endl`

- [ ] **Step 2: Fix `src/crypto_utils.cpp`**

Delete line 9 (`using namespace std;`). Replace all unqualified names. Key patterns:

- `string` → `std::string`
- `vector` → `std::vector`
- `cout` → `std::cout`
- `cerr` → `std::cerr`
- `endl` → `std::endl`
- `random_device` → `std::random_device`
- `mt19937` → `std::mt19937`
- `uniform_int_distribution` → `std::uniform_int_distribution`
- `stringstream` → `std::stringstream`
- `hex` → `std::hex`
- `setw` → `std::setw`
- `setfill` → `std::setfill`
- `min` → `std::min` (currently not used in crypto_utils.cpp — check for `min`)
- `map` → (not in crypto_utils.cpp headers — ok)

- [ ] **Step 3: Fix `src/image_utils.cpp`**

Delete line 12 (`using namespace std;`). Replace all unqualified names:

- `string` → `std::string`
- `vector` → `std::vector`
- `cout` → `std::cout`
- `endl` → `std::endl`
- `random_device` → `std::random_device`
- `mt19937` → `std::mt19937`
- `uniform_int_distribution` → `std::uniform_int_distribution`
- `uniform_real_distribution` → `std::uniform_real_distribution`
- `normal_distribution` → `std::normal_distribution`
- `map` → `std::map`
- `shuffle` → `std::shuffle`
- `sort` → `std::sort`
- `min` → `std::min`
- `max` → `std::max`
- `sqrt` → `std::sqrt`
- `stringstream` → (check if used — line 59 in crypto not image)

- [ ] **Step 4: Verify compilation**

```bash
make clean && make
```

Expected: compiles without errors, no warnings about missing `std::`.

- [ ] **Step 5: Run smoke test**

```bash
echo -e "1\ntestpass\ntestpass\nmysecret\n3\n" | ./passpix
```

Expected: generates `enc_*.png` output successfully.

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp src/crypto_utils.cpp src/image_utils.cpp
git commit -m "refactor: remove using namespace std from all source files"
```

---

### Task 6: Replace `dirent.h` with `std::filesystem`

**Files:**
- Modify: `src/image_utils.cpp`

**Interfaces:**
- Consumes: C++17 (from Task 1)
- Produces: `std::vector<std::string> listEncFiles()` — same signature, cross-platform implementation

- [ ] **Step 1: Replace `listEncFiles()` implementation**

In `src/image_utils.cpp`:

Replace line 6: `#include <dirent.h>` with `#include <filesystem>`

Replace the entire `listEncFiles()` function (lines 451-469) with:

```cpp
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
```

- [ ] **Step 2: Ensure `<filesystem>` is linked**

On Linux with g++ 8+, `std::filesystem` is in `libstdc++`. No extra link flags needed for g++ 9+. For g++ 8, add `-lstdc++fs` to link flags.

Check g++ version:
```bash
g++ --version
```

If g++ 8.x, add to Makefile LDFLAGS: `LDFLAGS += -lstdc++fs` (line 46) and to CMakeLists.txt: `target_link_libraries(passpix PRIVATE stdc++fs)`

- [ ] **Step 3: Verify compilation**

```bash
make clean && make
```

- [ ] **Step 4: Verify file listing works**

```bash
./passpix
# Select option 2 (decrypt) — should list enc_*.png files without crashing
```

- [ ] **Step 5: Commit**

```bash
git add src/image_utils.cpp
git commit -m "refactor: replace dirent.h with std::filesystem for cross-platform file listing"
```

---

### Task 7: Crypto unit tests

**Files:**
- Modify: `test/test_crypto.cpp`

**Interfaces:**
- Consumes: `crypto_utils.h` functions: `sha256`, `pbkdf2`, `aesEncrypt`, `aesDecrypt`, `generateHMAC`, `verifyHMAC`, `deriveSeedFromKey`, `generateRandomString`
- Produces: Passing unit tests for all crypto primitives

- [ ] **Step 1: Write crypto unit tests**

Replace `test/test_crypto.cpp`:

```cpp
#include "catch_amalgamated.hpp"
#include "crypto_utils.h"
#include <cstring>

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

TEST_CASE("AES-256-CBC encrypt then decrypt roundtrip", "[crypto]") {
    std::string key = generateRandomString(AES_KEY_SIZE);
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);

    SECTION("short plaintext") {
        std::string plaintext = "hello";
        std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
        REQUIRE_FALSE(ciphertext.empty());
        std::string decrypted = aesDecrypt(ciphertext, key, iv);
        REQUIRE(decrypted == plaintext);
    }

    SECTION("exact block size plaintext") {
        std::string plaintext(16, 'A');
        std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
        REQUIRE_FALSE(ciphertext.empty());
        std::string decrypted = aesDecrypt(ciphertext, key, iv);
        REQUIRE(decrypted == plaintext);
    }

    SECTION("multi-block plaintext") {
        std::string plaintext(47, 'B');
        std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
        REQUIRE_FALSE(ciphertext.empty());
        REQUIRE(ciphertext.size() >= plaintext.size());
        std::string decrypted = aesDecrypt(ciphertext, key, iv);
        REQUIRE(decrypted == plaintext);
    }
}

TEST_CASE("AES decryption with wrong key fails", "[crypto]") {
    std::string key = generateRandomString(AES_KEY_SIZE);
    std::string wrongKey = generateRandomString(AES_KEY_SIZE);
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);

    std::string plaintext = "secret data";
    std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
    std::string decrypted = aesDecrypt(ciphertext, wrongKey, iv);
    REQUIRE(decrypted != plaintext);
}

TEST_CASE("AES decryption with wrong IV fails", "[crypto]") {
    std::string key = generateRandomString(AES_KEY_SIZE);
    std::vector<unsigned char> iv(AES_BLOCK_SIZE);
    std::vector<unsigned char> wrongIv(AES_BLOCK_SIZE);
    RAND_bytes(iv.data(), AES_BLOCK_SIZE);
    RAND_bytes(wrongIv.data(), AES_BLOCK_SIZE);

    std::string plaintext = "secret data";
    std::vector<unsigned char> ciphertext = aesEncrypt(plaintext, key, iv);
    std::string decrypted = aesDecrypt(ciphertext, wrongIv, key);
    REQUIRE(decrypted != plaintext);
}

TEST_CASE("PBKDF2 produces deterministic output", "[crypto]") {
    std::string password = "testpassword";
    std::string salt = "abcdefghijklmnop"; // 16 bytes
    size_t keyLen = 32;

    std::string key1 = pbkdf2(password, salt, keyLen, PBKDF2_ITERATIONS);
    std::string key2 = pbkdf2(password, salt, keyLen, PBKDF2_ITERATIONS);

    REQUIRE_FALSE(key1.empty());
    REQUIRE(key1.size() == keyLen);
    REQUIRE(key1 == key2);
}

TEST_CASE("PBKDF2 produces different output with different salts", "[crypto]") {
    std::string password = "testpassword";
    std::string salt1 = "abcdefghijklmnop";
    std::string salt2 = "qrstuvwxyz012345"; // different 16-byte salt

    std::string key1 = pbkdf2(password, salt1, 32, PBKDF2_ITERATIONS);
    std::string key2 = pbkdf2(password, salt2, 32, PBKDF2_ITERATIONS);

    REQUIRE(key1 != key2);
}

TEST_CASE("HMAC generation and verification", "[crypto]") {
    std::string key = generateRandomString(32);
    std::vector<unsigned char> data = {'H', 'e', 'l', 'l', 'o'};

    std::vector<unsigned char> hmac = generateHMAC(data, key);
    REQUIRE(hmac.size() == HMAC_SIZE);
    REQUIRE(verifyHMAC(data, hmac, key));
}

TEST_CASE("HMAC verification fails with wrong key", "[crypto]") {
    std::string key1 = generateRandomString(32);
    std::string key2 = generateRandomString(32);
    while (key1 == key2) key2 = generateRandomString(32);

    std::vector<unsigned char> data = {'t', 'e', 's', 't'};
    std::vector<unsigned char> hmac = generateHMAC(data, key1);

    REQUIRE_FALSE(verifyHMAC(data, hmac, key2));
}

TEST_CASE("HMAC verification fails with tampered data", "[crypto]") {
    std::string key = generateRandomString(32);
    std::vector<unsigned char> data = {'o', 'r', 'i', 'g', 'i', 'n', 'a', 'l'};
    std::vector<unsigned char> hmac = generateHMAC(data, key);

    std::vector<unsigned char> tampered = {'t', 'a', 'm', 'p', 'e', 'r', 'e', 'd'};
    REQUIRE_FALSE(verifyHMAC(tampered, hmac, key));
}

TEST_CASE("deriveSeedFromKey is deterministic", "[crypto]") {
    std::string key = generateRandomString(32);
    std::string salt = generateRandomString(16);

    unsigned seed1 = deriveSeedFromKey(key, salt);
    unsigned seed2 = deriveSeedFromKey(key, salt);

    REQUIRE(seed1 == seed2);
}

TEST_CASE("deriveSeedFromKey produces different seeds for different inputs", "[crypto]") {
    std::string key = generateRandomString(32);
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
```

- [ ] **Step 2: Build and run tests**

```bash
make test
```

Expected: all crypto tests pass.

- [ ] **Step 3: Commit**

```bash
git add test/test_crypto.cpp
git commit -m "test: add unit tests for all crypto functions"
```

---

### Task 8: Integration tests (stego roundtrip) + CLI smoke test

**Files:**
- Modify: `test/test_stego.cpp`
- Create: `test/smoke_test.sh`

**Interfaces:**
- Consumes: `encryptPassword`, `decryptPassword` from `image_utils.h`
- Produces: Passing integration tests covering full encrypt-decrypt cycle

- [ ] **Step 1: Write integration tests**

Replace `test/test_stego.cpp`:

```cpp
#include "catch_amalgamated.hpp"
#include "image_utils.h"
#include <filesystem>
#include <fstream>

static std::string getGeneratedFilename() {
    namespace fs = std::filesystem;
    for (const auto& entry : fs::directory_iterator(".")) {
        std::string name = entry.path().filename().string();
        if (name.size() > 4 && name.substr(0, 4) == "enc_" && name.substr(name.size() - 4) == ".png") {
            return name;
        }
    }
    return "";
}

TEST_CASE("Full encrypt-decrypt roundtrip", "[integration]") {
    std::string masterPass = "integration-test-master-passphrase";
    std::string password = "my-secret-password-123";

    // Clean up any existing enc_ files from prior runs
    std::string existing = getGeneratedFilename();
    while (!existing.empty()) {
        std::filesystem::remove(existing);
        existing = getGeneratedFilename();
    }

    encryptPassword(masterPass, password);

    std::string filename = getGeneratedFilename();
    REQUIRE_FALSE(filename.empty());
    REQUIRE(std::filesystem::exists(filename));

    std::string decrypted = decryptPassword(masterPass, filename);
    REQUIRE(decrypted == password);

    std::filesystem::remove(filename);
}

TEST_CASE("Decrypt with wrong passphrase produces wrong result", "[integration]") {
    std::string masterPass = "correct-master-passphrase";
    std::string wrongPass = "wrong-master-passphrase";
    std::string password = "another-secret";

    std::string existing = getGeneratedFilename();
    while (!existing.empty()) {
        std::filesystem::remove(existing);
        existing = getGeneratedFilename();
    }

    encryptPassword(masterPass, password);
    std::string filename = getGeneratedFilename();
    REQUIRE_FALSE(filename.empty());

    std::string decrypted = decryptPassword(wrongPass, filename);
    REQUIRE(decrypted != password);

    std::filesystem::remove(filename);
}

TEST_CASE("Encrypted PNG is valid and has correct dimensions", "[integration]") {
    std::string masterPass = "dimension-test-passphrase";
    std::string password = "test-password";

    std::string existing = getGeneratedFilename();
    while (!existing.empty()) {
        std::filesystem::remove(existing);
        existing = getGeneratedFilename();
    }

    encryptPassword(masterPass, password);
    std::string filename = getGeneratedFilename();
    REQUIRE_FALSE(filename.empty());

    // Verify PNG decodes correctly and has expected dimensions
    std::vector<unsigned char> img;
    unsigned w, h;
    unsigned error = lodepng::decode(img, w, h, filename);
    REQUIRE(error == 0);
    REQUIRE(w == 1920);
    REQUIRE(h == 1080);

    std::filesystem::remove(filename);
}

TEST_CASE("Generated files are named enc_<random>.png", "[integration]") {
    std::string masterPass = "naming-test";
    std::string password = "test";

    std::string existing = getGeneratedFilename();
    while (!existing.empty()) {
        std::filesystem::remove(existing);
        existing = getGeneratedFilename();
    }

    encryptPassword(masterPass, password);
    std::string filename = getGeneratedFilename();
    REQUIRE_FALSE(filename.empty());
    REQUIRE(filename.substr(0, 4) == "enc_");
    REQUIRE(filename.substr(filename.size() - 4) == ".png");

    std::filesystem::remove(filename);
}
```

- [ ] **Step 2: Create CLI smoke test script**

Write `test/smoke_test.sh`:

```bash
#!/usr/bin/env bash
set -euo pipefail

BINARY="${1:-./passpix}"
PASS="smoke-test-master"
SECRET="smoke-secret-123"
TESTDIR=$(mktemp -d)
trap "rm -rf $TESTDIR" EXIT

cd "$TESTDIR"
cp "$OLDPWD/$BINARY" ./passpix 2>/dev/null || cp "$OLDPWD/$BINARY" ./passpix
BIN=./passpix

# Test encrypt
echo -e "1\n$PASS\n$PASS\n$SECRET\n3\n" | $BIN 2>&1 | grep -q "Password encrypted"
echo "PASS: encryption"

# Get generated filename
ENC_FILE=$(ls enc_*.png 2>/dev/null | head -1)
if [ -z "$ENC_FILE" ]; then
    echo "FAIL: no encrypted file generated"
    exit 1
fi
echo "PASS: file generated: $ENC_FILE"

# Test decrypt
RESULT=$(echo -e "2\n1\n$PASS\n3\n" | $BIN 2>&1)
echo "$RESULT" | grep -q "$SECRET"
echo "PASS: decryption returned correct password"

echo "All smoke tests passed."
```

```bash
chmod +x test/smoke_test.sh
```

- [ ] **Step 3: Run all tests**

```bash
make test
```

Expected: all crypto + integration tests pass. Then:

```bash
./test/smoke_test.sh ./passpix
```

Expected: "All smoke tests passed."

- [ ] **Step 4: Commit**

```bash
git add test/test_stego.cpp test/smoke_test.sh
git commit -m "test: add integration tests and CLI smoke test"
```

---

### Task 9: Split `image_utils.cpp` into `image_gen.cpp` + `stego.cpp`

**Files:**
- Create: `src/image_gen.h`
- Create: `src/image_gen.cpp`
- Create: `src/stego.h`
- Create: `src/stego.cpp`
- Modify: `src/image_utils.h` (become compatibility wrapper)
- Modify: `src/image_utils.cpp` (become compatibility wrapper)
- Modify: `src/main.cpp` (update include if needed)

**Interfaces:**
- Produces:
  - `image_gen.h`: `void generateGradient(...)`, `void addNaturalNoise(...)`, `void addShapes(...)`, `const unsigned IMAGE_WIDTH`, `const unsigned IMAGE_HEIGHT`
  - `stego.h`: `void embedBytes(...)`, `std::vector<unsigned char> extractBytes(...)`, metadata constants
  - `image_utils.h`: unchanged public API (`encryptPassword`, `decryptPassword`, `listEncFiles`)
- Consumes: `crypto_utils.h` (stego depends on this for seed derivation), `lodepng.h` (image_gen depends for encode)

- [ ] **Step 1: Create `src/image_gen.h`**

```cpp
#pragma once

#include <vector>
#include <random>

const unsigned IMAGE_WIDTH = 1920;
const unsigned IMAGE_HEIGHT = 1080;

void generateGradient(std::vector<unsigned char>& image, unsigned width, unsigned height,
                      const std::vector<unsigned char>& color1,
                      const std::vector<unsigned char>& color2);
void addNaturalNoise(std::vector<unsigned char>& image, unsigned width, unsigned height,
                     float intensity);
void addShapes(std::vector<unsigned char>& image, unsigned width, unsigned height,
               int numShapes, std::mt19937& rng);
```

- [ ] **Step 2: Create `src/image_gen.cpp`**

Extract from `image_utils.cpp`: the `generateGradient`, `addNaturalNoise`, and `addShapes` functions. Move `IMAGE_WIDTH` and `IMAGE_HEIGHT` constants into the header. Move includes needed by these functions: `<iostream>`, `<random>`, `<algorithm>`, `<cmath>`.

```cpp
#include "image_gen.h"
#include <algorithm>
#include <cmath>
#include <random>

void generateGradient(std::vector<unsigned char>& image, unsigned width, unsigned height,
                      const std::vector<unsigned char>& color1,
                      const std::vector<unsigned char>& color2) {
    const float widthInv = 1.0f / static_cast<float>(width);
    const float heightInv = 1.0f / static_cast<float>(height);

    for (unsigned y = 0; y < height; y++) {
        const float factor2 = static_cast<float>(y) * heightInv;
        const size_t rowOffset = static_cast<size_t>(y) * width * 4;

        for (unsigned x = 0; x < width; x++) {
            const float factor = static_cast<float>(x) * widthInv;
            const float blend = 0.5f * (factor + factor2);
            const float oneMinusBlend = 1.0f - blend;

            const size_t idx = rowOffset + static_cast<size_t>(x) * 4;
            image[idx]     = static_cast<unsigned char>(static_cast<float>(color1[0]) * oneMinusBlend + static_cast<float>(color2[0]) * blend);
            image[idx + 1] = static_cast<unsigned char>(static_cast<float>(color1[1]) * oneMinusBlend + static_cast<float>(color2[1]) * blend);
            image[idx + 2] = static_cast<unsigned char>(static_cast<float>(color1[2]) * oneMinusBlend + static_cast<float>(color2[2]) * blend);
            image[idx + 3] = static_cast<unsigned char>(static_cast<float>(color1[3]) * oneMinusBlend + static_cast<float>(color2[3]) * blend);
        }
    }
}

void addNaturalNoise(std::vector<unsigned char>& image, unsigned width, unsigned height,
                     float intensity) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::normal_distribution<float> dist(0.0f, intensity);

    const size_t totalPixels = static_cast<size_t>(width) * height;
    for (size_t pixel = 0; pixel < totalPixels; pixel++) {
        const size_t idx = pixel * 4;
        const int newR = static_cast<int>(image[idx])     + static_cast<int>(dist(rng));
        const int newG = static_cast<int>(image[idx + 1]) + static_cast<int>(dist(rng));
        const int newB = static_cast<int>(image[idx + 2]) + static_cast<int>(dist(rng));

        image[idx]     = static_cast<unsigned char>(std::max(0, std::min(255, newR)));
        image[idx + 1] = static_cast<unsigned char>(std::max(0, std::min(255, newG)));
        image[idx + 2] = static_cast<unsigned char>(std::max(0, std::min(255, newB)));
    }
}

void addShapes(std::vector<unsigned char>& image, unsigned width, unsigned height,
               int numShapes, std::mt19937& rng) {
    std::uniform_int_distribution<int> xDist(0, static_cast<int>(width) - 1);
    std::uniform_int_distribution<int> yDist(0, static_cast<int>(height) - 1);
    std::uniform_int_distribution<int> radiusDist(30, 150);
    std::uniform_int_distribution<int> colorDist(0, 255);
    std::uniform_real_distribution<float> opacityDist(0.1f, 0.3f);

    for (int s = 0; s < numShapes; s++) {
        const int centerX = xDist(rng);
        const int centerY = yDist(rng);
        const int radius = radiusDist(rng);
        const unsigned char shapeColor[3] = {
            static_cast<unsigned char>(colorDist(rng)),
            static_cast<unsigned char>(colorDist(rng)),
            static_cast<unsigned char>(colorDist(rng))
        };
        const float opacity = opacityDist(rng);

        const int minY = std::max(0, centerY - radius);
        const int maxY = std::min(static_cast<int>(height), centerY + radius);
        const int minX = std::max(0, centerX - radius);
        const int maxX = std::min(static_cast<int>(width), centerX + radius);
        const float radiusSquared = static_cast<float>(radius * radius);

        for (int y = minY; y < maxY; y++) {
            const int dy = y - centerY;
            const int dySquared = dy * dy;
            const size_t rowOffset = static_cast<size_t>(y) * width * 4;

            for (int x = minX; x < maxX; x++) {
                const int dx = x - centerX;
                const float distanceSquared = static_cast<float>(dx * dx + dySquared);

                if (distanceSquared < radiusSquared) {
                    const float distance = std::sqrt(distanceSquared);
                    float factor = 1.0f - (distance / static_cast<float>(radius));
                    factor = factor * factor * opacity;
                    const float oneMinusFactor = 1.0f - factor;

                    const size_t idx = rowOffset + static_cast<size_t>(x) * 4;
                    image[idx]     = static_cast<unsigned char>(static_cast<float>(image[idx])     * oneMinusFactor + static_cast<float>(shapeColor[0]) * factor);
                    image[idx + 1] = static_cast<unsigned char>(static_cast<float>(image[idx + 1]) * oneMinusFactor + static_cast<float>(shapeColor[1]) * factor);
                    image[idx + 2] = static_cast<unsigned char>(static_cast<float>(image[idx + 2]) * oneMinusFactor + static_cast<float>(shapeColor[2]) * factor);
                }
            }
        }
    }
}
```

- [ ] **Step 3: Create `src/stego.h`**

```cpp
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
```

- [ ] **Step 4: Create `src/stego.cpp`**

Extract the embedding and extraction logic from `encryptPassword` and `decryptPassword` in `image_utils.cpp`.

```cpp
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
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
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

    // Store HMAC at three locations
    const size_t hmacSize = std::min(payload.hmac.size(), static_cast<size_t>(HMAC_SIZE));
    for (size_t i = 0; i < hmacSize; i++) {
        const unsigned char hmacByte = payload.hmac[i];
        image[imageSize - HMAC_REAR_OFFSET - i] = hmacByte;
        image[imageSize - HMAC_REAR_OFFSET - HMAC_SIZE - i] = hmacByte;
        image[HMAC_OFFSET + i] = hmacByte;
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
    result.iv.resize(AES_BLOCK_SIZE);
    for (int i = 0; i < AES_BLOCK_SIZE; i++) {
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

    // Extract HMAC from 3 locations
    result.hmac.resize(HMAC_SIZE);
    for (int i = 0; i < HMAC_SIZE; i++) {
        result.hmac[i] = image[imageSize - HMAC_REAR_OFFSET - i];
    }

    // Extract stored password hash
    result.passwordHash.reserve(32);
    for (size_t i = 0; i < 32 && i + HASH_OFFSET < imageSize; i++) {
        result.passwordHash.push_back(static_cast<char>(image[HASH_OFFSET + i]));
    }

    return result;
}
```

- [ ] **Step 5: Rewrite `src/image_utils.cpp` as compatibility wrapper**

Replace `image_utils.cpp` to use the new split modules:

```cpp
#include "image_utils.h"
#include "image_gen.h"
#include "stego.h"
#include "crypto_utils.h"
#include "../Include/lodepng.h"
#include <iostream>
#include <random>
#include <filesystem>
#include <algorithm>

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
    std::string salt = generateRandomString(16);
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

    std::string filename = "enc_" + generateRandomString(10) + ".png";
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
    salt.reserve(16);
    for (size_t i = 0; i < 16; i++) {
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
```

- [ ] **Step 6: Update `src/image_utils.h`**

Replace with:

```cpp
#pragma once

#include <vector>
#include <string>

void encryptPassword(const std::string& masterPassphrase, const std::string& password);
std::string decryptPassword(const std::string& masterPassphrase, const std::string& filename);
std::vector<std::string> listEncFiles();
```

- [ ] **Step 7: Verify compilation and tests**

```bash
make clean && make && make test
```

Also run:
```bash
./test/smoke_test.sh ./passpix
```

- [ ] **Step 8: Commit**

```bash
git add src/image_gen.h src/image_gen.cpp src/stego.h src/stego.cpp src/image_utils.h src/image_utils.cpp
git commit -m "refactor: split image_utils into image_gen + stego, keep compat wrapper"
```

---

### Task 10: Update build system for split files

**Files:**
- Modify: `Makefile`
- Modify: `CMakeLists.txt`
- Modify: `test/Makefile`

**Interfaces:**
- Consumes: New source files from Task 9
- Produces: Build system compiles all split modules correctly

- [ ] **Step 1: Update root Makefile**

Change SOURCES in `Makefile` (lines 32-35) from:
```
SOURCES = src/main.cpp \
          src/crypto_utils.cpp \
          src/image_utils.cpp \
          Include/lodepng.cpp
```
to:
```
SOURCES = src/main.cpp \
          src/crypto_utils.cpp \
          src/image_gen.cpp \
          src/stego.cpp \
          src/image_utils.cpp \
          Include/lodepng.cpp
```

- [ ] **Step 2: Update CMakeLists.txt**

Change SOURCES in `CMakeLists.txt` (lines 56-61) from:
```cmake
set(SOURCES
    src/main.cpp
    src/crypto_utils.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)
```
to:
```cmake
set(SOURCES
    src/main.cpp
    src/crypto_utils.cpp
    src/image_gen.cpp
    src/stego.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)
```

- [ ] **Step 3: Update test/Makefile SRC_SOURCES**

Change `test/Makefile` line from:
```
SRC_SOURCES = ../src/crypto_utils.cpp ../src/image_utils.cpp ../Include/lodepng.cpp
```
to:
```
SRC_SOURCES = ../src/crypto_utils.cpp ../src/image_gen.cpp ../src/stego.cpp ../src/image_utils.cpp ../Include/lodepng.cpp
```

- [ ] **Step 4: Update CMakeLists.txt test sources**

In `CMakeLists.txt`, update the TEST_SOURCES block (added in Task 4 Step 4) from:
```cmake
set(TEST_SOURCES
    test/catch_amalgamated.cpp
    test/test_crypto.cpp
    test/test_stego.cpp
    src/crypto_utils.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)
```
to:
```cmake
set(TEST_SOURCES
    test/catch_amalgamated.cpp
    test/test_crypto.cpp
    test/test_stego.cpp
    src/crypto_utils.cpp
    src/image_gen.cpp
    src/stego.cpp
    src/image_utils.cpp
    Include/lodepng.cpp
)
```

- [ ] **Step 5: Verify full build + tests**

```bash
make clean && make && make test
```

Also test CMake:
```bash
mkdir -p build/cmake-test && cd build/cmake-test && cmake ../.. && make && ctest
```

- [ ] **Step 6: Verify CLI smoke test still works**

```bash
./test/smoke_test.sh ./passpix
```

- [ ] **Step 7: Commit**

```bash
git add Makefile CMakeLists.txt test/Makefile
git commit -m "build: update build system for split image_gen/stego modules"
```

---

### Task 11: Minor cleanup — constants, const, error handling

**Files:**
- Modify: `src/image_gen.cpp`
- Modify: `src/stego.cpp`
- Modify: `src/crypto_utils.cpp`
- Modify: `src/main.cpp`

**Interfaces:**
- No API changes
- Produces: Cleaner code with named constants, const-correctness, better error handling

- [ ] **Step 1: Extract magic numbers in `src/image_gen.cpp`**

Add constants at top of file:
```cpp
namespace {
    constexpr float NOISE_INTENSITY = 10.0f;
    constexpr int MIN_SHAPES = 10;
    constexpr int MAX_SHAPES = 25;
    constexpr int MIN_RADIUS = 30;
    constexpr int MAX_RADIUS = 150;
    constexpr float MIN_OPACITY = 0.1f;
    constexpr float MAX_OPACITY = 0.3f;
}
```

Update distribution ranges to use these constants in `addShapes`:
- `radiusDist(30, 150)` → `radiusDist(MIN_RADIUS, MAX_RADIUS)`
- `opacityDist(0.1f, 0.3f)` → `opacityDist(MIN_OPACITY, MAX_OPACITY)`

- [ ] **Step 2: Add const where applicable in `src/stego.cpp`**

Audit functions — many `width`, `height`, `image` parameters are not mutated. Add `const`:
- `embedPayload(std::vector<unsigned char>& image, unsigned width, unsigned height, const EncryptedPayload& payload, const std::string& key)` — image stays mutable (it's the output), but width/height are already value params
- `extractPayload(const std::vector<unsigned char>& image, unsigned width, unsigned height, const std::string& key)` — image is already const

- [ ] **Step 3: Add string length constants in `src/image_utils.cpp`**

```cpp
namespace {
    constexpr size_t FILENAME_RANDOM_LENGTH = 10;
    constexpr size_t SALT_LENGTH = 16;
}
```

Use `SALT_LENGTH` where `16` is hardcoded for salt. Use `FILENAME_RANDOM_LENGTH` where `10` is used for filename generation.

- [ ] **Step 4: Improve error handling in `src/crypto_utils.cpp`**

In `pbkdf2()`: change `cerr << "Error generating key..."` to also report the error via a `std::cerr` with more detail:
```cpp
if (PKCS5_PBKDF2_HMAC(...) != 1) {
    std::cerr << "Error: PBKDF2 key derivation failed" << std::endl;
    return "";
}
```

- [ ] **Step 5: Verify compilation and tests**

```bash
make clean && make && make test
```

- [ ] **Step 6: Commit**

```bash
git add src/image_gen.cpp src/stego.cpp src/image_utils.cpp src/crypto_utils.cpp
git commit -m "refactor: extract magic numbers into named constants, improve const-correctness"
```

---

### Task 12: Final verification

**Files:**
- None (verification only)

**Interfaces:**
- Consumes: Everything from Tasks 1-11

- [ ] **Step 1: Clean build from scratch**

```bash
make clean && make
```

Expected: zero warnings, zero errors.

- [ ] **Step 2: Run all tests**

```bash
make test
```

Expected: all unit + integration tests pass.

- [ ] **Step 3: Run CLI smoke test**

```bash
./test/smoke_test.sh ./passpix
```

Expected: "All smoke tests passed."

- [ ] **Step 4: Verify CMake build**

```bash
mkdir -p /tmp/passpix-cmake-build && cd /tmp/passpix-cmake-build
cmake /home/fly/Pro/PassPix && make && ctest --output-on-failure
```

Expected: builds cleanly, all CTest tests pass.

- [ ] **Step 5: Verify no `using namespace std`**

```bash
grep -r "using namespace std" src/ test/*.cpp
```

Expected: no matches in any source or test file (only inside `namespace fs =` aliases, which are fine).

- [ ] **Step 6: Verify LICENSE exists and README is correct**

```bash
head -3 LICENSE
grep "1920×1080" README.md
grep "MIT" README.md
```

Expected: all return matches.

- [ ] **Step 7: Check for leftover enc_*.png from testing**

```bash
rm -f enc_*.png
```

- [ ] **Step 8: Final commit if needed**

```bash
git status
```

If clean, no commit needed. If enc_*.png were removed, commit .gitignore update.

---

## Summary

**Total: 12 tasks** across 3 phases:

| Task | Phase | Description |
|---|---|---|
| 1 | Foundation | C++17 bump + Makefile doc fixes |
| 2 | Foundation | MIT License + README updates |
| 3 | Foundation | CI Pipeline (GitHub Actions) |
| 4 | Testing | Vend Catch2 + test infrastructure |
| 5 | Refactor | Remove `using namespace std` |
| 6 | Refactor | Replace `dirent.h` with `std::filesystem` |
| 7 | Testing | Crypto unit tests |
| 8 | Testing | Integration tests + CLI smoke test |
| 9 | Refactor | Split `image_utils.cpp` → `image_gen.cpp` + `stego.cpp` |
| 10 | Build | Update build system for split files |
| 11 | Refactor | Minor cleanup (constants, const, error handling) |
| 12 | Verify | Final verification |

**Dependency chains:**
- Tasks 1-3 are independent of each other (can run in parallel)
- Task 4 depends on Task 1 (C++17)
- Tasks 5, 6 depend on Task 1 (C++17)
- Task 7 depends on Task 4 (Catch2) and Task 5 (namespace fix enables clean test compilation)
- Task 8 depends on Tasks 4, 5, 6, 7
- Task 9 depends on Tasks 5, 6 (clean filesystem API) — and Task 8 to verify behavior
- Task 10 depends on Task 9
- Task 11 depends on Task 9
- Task 12 depends on all
