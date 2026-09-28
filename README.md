# PassPix

**Encrypt passwords and hide them inside natural-looking PNG images.**

PassPix generates a unique abstract gradient image, encrypts your password with XChaCha20-Poly1305, then steganographically embeds the ciphertext into the image's pixels. The resulting PNG looks like a normal abstract photo — only someone with the correct master passphrase can extract the secret back out.

> ⚠️ **Beta software.** Use at your own risk. Review the source before trusting sensitive data.

---

## How It Works

```
You enter a master passphrase + password to store
       ↓
    PassPix generates a random 1920×1080 gradient image
    with soft shapes and natural noise
       ↓
    Your password is encrypted with XChaCha20-Poly1305
    (key derived from your master passphrase via Argon2id with 64 MiB memory)
       ↓
    The ciphertext + salt + nonce are embedded
    16 times at pseudo-random pixel positions
       ↓
    Output: a .png file that looks like abstract art
```

To recover the password, you provide the same master passphrase — the correct key is re-derived, and the ciphertext is extracted and decrypted.

---

## Pre-built Binaries

Pre-compiled binaries are available on the [Releases](https://github.com/TurcanS/PassPix/releases) page. To verify integrity, check that your downloaded file matches the SHA-256 hash below:

> **Note:** These hashes are for the CI-built release binaries compiled with `-march=x86-64-v2`. Locally compiled binaries will have different hashes due to `-march=native` CPU-specific optimizations.

<!-- RELEASE_TABLE -->
| Version | Platform | Binary | SHA-256 Hash |
|---|---|---|---|
| <!-- VERSION -->v0.3.0<!-- /VERSION --> | Linux | `passpix` | <!-- HASH_LINUX -->`f1745d715ef787408370cf19a0433ff835da2176fc2fc257ebc9b3ed8d337d33`<!-- /HASH_LINUX --> |
| <!-- VERSION -->v0.3.0<!-- /VERSION --> | Windows | `passpix.exe` | <!-- HASH_WIN -->`2feb96da5e56f404eb5023440060fd7c42abe45c14cfeb9ce95a3e21747fbd8c`<!-- /HASH_WIN --> |

---

## Compile from Source

### Dependencies

- **C++ compiler**: g++ or clang++ with C++17 support
- **libsodium**: 1.0.18 or newer (Argon2id, XChaCha20-Poly1305, SHA-256)
  - Debian/Ubuntu: `libsodium-dev`
  - Arch: `libsodium`
  - Windows (MSYS2): `mingw-w64-ucrt-x86_64-libsodium`
- **Make** or **CMake** (3.10+)
- Optional, for clipboard support on Linux: `wl-clipboard` (Wayland), `xclip` or `xsel` (X11)

OpenSSL is no longer needed as of v0.4.0.

### Build

#### Linux

```bash
# Install dependencies
sudo apt install build-essential libsodium-dev

# Clone and build
git clone https://github.com/TurcanS/PassPix.git
cd PassPix
make

# Run
./passpix

# Run tests
make test
```

#### Windows (MSYS2)

```bash
# Install MSYS2 from https://www.msys2.org/

# Open UCRT64 terminal and install dependencies
pacman -S --needed base-devel mingw-w64-ucrt-x86_64-toolchain \
  mingw-w64-ucrt-x86_64-libsodium

# Clone and build
git clone https://github.com/TurcanS/PassPix.git
cd PassPix
make

# Run
./passpix

# Run tests
make test
```

#### Linux with CMake

```bash
sudo apt install build-essential libsodium-dev cmake pkg-config
git clone https://github.com/TurcanS/PassPix.git
cd PassPix
cmake -S . -B build-cmake
cmake --build build-cmake
ctest --test-dir build-cmake --output-on-failure
```

#### Build options

| Command | Effect |
|---|---|
| `make` | Optimized build for this CPU (`-march=native`) |
| `make PORTABLE=1` | Generic x86-64-v2 build, as used for release binaries |
| `make DEBUG=1` | Debug symbols, no optimization |
| `make SANITIZE=1` | Debug build with AddressSanitizer and UndefinedBehaviorSanitizer |
| `make WERROR=1` | Treat compiler warnings as errors |
| `make test` | Unit, integration and smoke tests |

### Usage

#### Interactive menu

```bash
./passpix
```

1. **Encrypt Password**: enter your master passphrase twice, then the password to store (twice). The image is saved as `img_<random>.png` in the current folder.
2. **Decrypt Password**: pick an image from the list (`0` goes back), then enter your master passphrase. You get 3 attempts. Then either copy the password to the clipboard (cleared when you press Enter) or show it on screen (hidden again when you press Enter).
3. **Exit** (or `q`, or Ctrl+D).

Passphrases and passwords are never shown while you type. PassPix warns about master passphrases shorter than 12 characters.

#### Commands

```bash
passpix encrypt                    # prompts, then prints the new image name
passpix decrypt img_Ab12Cd34Ef.png # prints the stored password
passpix decrypt <image> --copy     # copies it to the clipboard instead
passpix list [folder]              # lists PassPix images (default: current folder)
passpix --help
passpix --version
```

Passphrases are always prompted, never passed as arguments, so they stay out of your shell history. When input is piped, PassPix reads it line by line, so the commands can be scripted. Prompts and messages go to stderr, and only the result goes to stdout.

Exit codes: `0` success, `1` failure (for example a wrong passphrase), `2` usage error.

Set `NO_COLOR=1` to turn off colored output.

### Testing

```bash
# Unit + integration tests, then the CLI smoke test
make test

# Same, under AddressSanitizer and UndefinedBehaviorSanitizer
make clean && make SANITIZE=1 test

# Or with CMake
ctest --test-dir build-cmake --output-on-failure
```

CI runs on every push and pull request:
- gcc and clang builds with warnings as errors
- a sanitizer build
- a CMake build
- a Windows (MinGW) build
- a cross-platform check that each platform can decrypt an image made on the other

Release builds run the full test suite and check that the binary version matches the tag before anything is published.

---

## Technical Details

| Component | Choice |
|---|---|
| **Encryption** | XChaCha20-Poly1305 AEAD (authenticated encryption) |
| **Key derivation** | Argon2id, 64 MiB memory, 2 iterations (libsodium "interactive" limits) |
| **Integrity** | Poly1305 MAC (built into the AEAD cipher) |
| **Steganography** | Pseudorandom byte embedding with 16× redundancy + majority-vote recovery |
| **Image format** | PNG via [LodePNG](https://lodev.org/lodepng/) |
| **Image size** | 1920×1080 RGBA (8-bit per channel), opaque |
| **Password length** | Up to 1024 characters |
| **Memory safety** | Keys locked with `sodium_mlock`; keys, passphrases and passwords zeroed after use |

### Image embedding layout

- **Payload length:** stored 8 times, in the first pixel of rows 0, 2, …, 14.
- **Salt and nonce:** each stored 8 times in the first ~250 pixels. They are read back by majority vote, so a few damaged copies are outvoted.
- **Signature:** a 4-byte signature is spread over 512 positions so PassPix can recognize its own images.
- **Encrypted payload:** written 16 times, across three color channels, at positions shuffled with a seed derived from the key and salt. Each byte is recovered by majority vote.

A 32-byte slot that used to hold a SHA-256 of the password now holds random bytes (see Security Notes). The layout itself is unchanged, so images from older versions still decrypt.

---

## Project Structure

```
PassPix/
├── src/
│   ├── main.cpp             # Interactive menu and subcommands
│   ├── terminal.h/.cpp      # Hidden input, prompts, colored output
│   ├── clipboard.h/.cpp     # Clipboard copy/clear (Wayland, X11, Windows)
│   ├── version.h            # Version string
│   ├── crypto_utils.h/.cpp  # Argon2id, XChaCha20-Poly1305, SHA-256, memory wiping
│   ├── image_gen.h/.cpp     # Gradient, shapes, noise generation
│   ├── stego.h/.cpp         # Steganographic embed/extract with redundancy
│   └── image_utils.h/.cpp   # Encrypt/decrypt a password to/from an image file
├── Include/
│   ├── lodepng.cpp          # PNG encoding/decoding (bundled)
│   └── lodepng.h
├── test/
│   ├── test_crypto.cpp      # Unit tests for crypto functions
│   ├── test_stego.cpp       # Integration tests for images and embedding
│   └── smoke_test.sh        # CLI smoke test (menu and subcommands)
├── .github/workflows/       # CI (tests) and CD (releases)
├── Makefile                 # Make build
├── CMakeLists.txt           # CMake build
├── LICENSE                  # MIT license
└── README.md
```

---

## Security Notes

- **Re-encrypt images made with v0.3.0 or earlier.** Those versions also stored an unsalted SHA-256 hash of the password inside the image. Anyone with such an image can test password guesses against it directly, without the master passphrase. Since v0.4.0 that slot holds random bytes and nothing is derived from the password without the key. Old images still decrypt; to remove the hash, decrypt each one, encrypt the password again, and delete the old image.
- **Master passphrase strength matters.** Use a long, unique passphrase: it's the single key to all stored passwords. Argon2id with 64 MiB memory makes brute-force attacks expensive, but a weak passphrase can still be cracked.
- **Key material is protected in memory.** Derived keys are locked from swapping (`sodium_mlock`), and keys, passphrases and passwords are zeroed after use (`sodium_memzero`).
- **Prefer the clipboard or hide the password after reading it.** A password printed to the terminal can stay in scrollback. The copy option clears the clipboard when you press Enter. On Windows it also keeps the password out of clipboard history.
- **The generated images are not encrypted containers.** The pixel data looks random but the embedding pattern is deterministic. Security relies on the cryptographic key, not obscurity.
- **Verify pre-built binaries.** If you use pre-compiled releases, verify the SHA-256 hash matches the table above before trusting the binary.

---

## License

MIT — see [LICENSE](LICENSE).
