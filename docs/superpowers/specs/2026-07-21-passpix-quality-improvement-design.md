# PassPix Quality Improvement — Design Spec

**Date:** 2026-07-21
**Scope:** Production hardening + code quality. No new features, no GUI work.
**License change:** Apache 2.0 → MIT

---

## Section 1: Foundation Fixes

### 1.1 License
- Replace deleted `LICENSE` with standard MIT license text
- Update `README.md` license reference from "Apache 2.0" → "MIT"

### 1.2 README
- Fix image size to match source: **1920×1080**
- Update build instructions to reflect both Makefile and CMake paths
- Add "Running Tests" section documenting `make test` and `ctest`
- Remove stale references

### 1.3 Build System Unification
- CMake `project()`: rename from `enc_dec` → `passpix`
- Ensure both Makefile and CMake produce binary named `passpix`
- Clean `build/` directory: remove stale `enc_dec` binary and old CMake artifacts
- Update `.gitignore` to cover all build outputs

### 1.4 CI Pipeline (GitHub Actions)
- Single workflow: `.github/workflows/ci.yml`
- **Linux build**: Ubuntu latest, g++, OpenSSL dev (apt), runs `make` and `make test`
- **Windows build**: Windows latest, MinGW g++ via MSYS2, OpenSSL, runs `make` and `make test`
- Trigger: push to `main`, pull requests to `main`
- Artifact: compiled binary uploaded on success

---

## Section 2: Testing

### 2.1 Framework
- **Catch2 v3** (header-only, single `catch_amalgamated.hpp` + `catch_amalgamated.cpp` vendored in `test/` or fetched via CMake FetchContent)
- Configured for both Makefile (`make test` target) and CMake (`enable_testing()` + `add_test()`)

### 2.2 Unit Tests — Crypto (`test/test_crypto.cpp`)
- **PBKDF2-HMAC-SHA256**: known-vector test (RFC 6070 or OpenSSL test vectors, 100k iterations)
- **AES-256-CBC roundtrip**: encrypt plaintext → decrypt ciphertext → assert equality
- **AES-256-CBC padding**: verify PKCS#7 padding is added and stripped correctly
- **SHA-256**: known-vector test (empty string, "abc")
- **HMAC-SHA256**: known-vector test, constant-time comparison (`CRYPTO_memcmp`)

### 2.3 Integration Tests — Stego Roundtrip (`test/test_stego.cpp`)
- **Full roundtrip**: `encryptPassword(...)` → `decryptPassword(...)` → assert recovered password matches
- **Tamper detection**: modify a single pixel byte in output PNG, verify `decryptPassword` fails (HMAC mismatch)
- **Image validity**: generated PNG passes `lodepng_decode32()` without error; dimensions match expected
- **Image size**: generated image has exactly `IMAGE_WIDTH × IMAGE_HEIGHT` pixels

### 2.4 CLI Smoke Test
- Shell script (`test/smoke_test.sh`) or Makefile target that runs the built binary:
  - Encrypt a password (pipe input)
  - Decrypt from the generated PNG
  - Assert exit code 0 and correct output
- Called by `make test` after unit/integration tests pass

---

## Section 3: Code Quality & Refactoring

### 3.1 Remove `using namespace std`
- Files affected: `src/main.cpp`, `src/crypto_utils.cpp`, `src/image_utils.cpp`
- Replace with explicit `std::` prefixes on all standard library identifiers
- `src/crypto_utils.h` and `src/image_utils.h` also checked — headers should never have `using namespace std`

### 3.2 Split `src/image_utils.cpp` (469 lines)
This file currently does two unrelated things: generates aesthetic images and hides/recovers ciphertext in pixel bytes.

**New file: `src/image_gen.h` / `src/image_gen.cpp`**
- `generateGradient(...)` — random two-color gradient fill
- `addShapes(...)` — soft random circles with varying color/opacity
- `addNaturalNoise(...)` — Gaussian noise
- No crypto dependency

**New file: `src/stego.h` / `src/stego.cpp`**
- `embedBytes(...)` — pseudorandom pixel shuffle, redundant byte embedding, metadata storage
- `extractBytes(...)` — reverse shuffle, majority-vote recovery, metadata extraction
- Depends on `crypto_utils.h` for seed derivation (SHA-256)
- Depends on `image_gen.h` to receive image data

### 3.3 Decouple Encryption from Image Generation
- `encryptPassword()` in current code both encrypts AND generates images
- After split: orchestration stays in `src/main.cpp` via a new internal `encryptPassword()` that coordinates:
  1. Generate image buffer via `image_gen`
  2. Encrypt password + compute metadata via `crypto_utils`
  3. Embed ciphertext via `stego::embedBytes`
  4. Encode PNG via LodePNG
- Same for `decryptPassword()`: decode PNG → extract via stego → decrypt via crypto_utils
- Each unit independently testable; `main.cpp` becomes a thin orchestrator

### 3.6 Build System Updates for Refactoring
- Makefile: add `src/image_gen.o`, `src/stego.o` objects; update dependency chain
- CMakeLists.txt: add new source files to `add_executable()`
- Remove `src/image_utils.cpp` and `src/image_utils.h` after split is verified

### 3.7 C++ Standard Bump
- C++11 → C++17 (required for `std::filesystem`)
- Update `-std=c++11` → `-std=c++17` in Makefile
- Update `CMAKE_CXX_STANDARD 11` → `CMAKE_CXX_STANDARD 17` in CMakeLists.txt
- Minimum g++/MinGW version: 8 (released 2018, widely available)

### 3.4 Cross-Platform File Listing
- Replace `dirent.h` (POSIX) with `std::filesystem::directory_iterator` (C++17)
- Works on Linux, Windows (MSVC/MinGW), and macOS
- Bump C++ standard from C++11 to C++17 in both Makefile and CMakeLists.txt

### 3.5 Minor Cleanup
- Move magic numbers into named constants (redundancy counts, alpha noise intensity, shape count range)
- Add `const` qualifiers to functions and parameters where no mutation occurs
- Add explicit error handling for edge cases: empty password, empty passphrase, corrupt/missing PNG file, write permission failure
- Add `-Wall -Wextra` to compiler flags (currently only `-O3 -march=native -flto`)

---

## Non-Goals (Explicitly Out of Scope)
- Qt5 GUI (the `gui-development` branch is untouched)
- New features (multi-password, clipboard, file attachment, etc.)
- Encryption algorithm changes
- Image size configurability (stays 1920×1080)
- Performance optimization beyond existing `-O3 -flto`

---

## Success Criteria
1. `make test` passes on Linux and Windows MinGW
2. CI green on both platforms
3. No `using namespace std` in any source file
4. `image_utils.cpp` is split into `image_gen.cpp` + `stego.cpp`
5. MIT LICENSE file present and README references it
6. README correctly documents 1920×1080 images
7. Zero compiler warnings with `-Wall -Wextra`

---

## Open Decisions
- **Catch2 version**: v3 amalgamated (vendored) vs CMake FetchContent. Vendored is simpler for Makefile users; FetchContent is cleaner for CMake-only users. Decision needed before implementation.
