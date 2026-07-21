# PassPix

**Encrypt passwords and hide them inside natural-looking PNG images.**

PassPix generates a unique abstract gradient image, encrypts your password with AES-256, then steganographically embeds the ciphertext into the image's pixels. The resulting PNG looks like a normal abstract photo — only someone with the correct master passphrase can extract the secret back out.

> ⚠️ **Beta software.** Use at your own risk. Review the source before trusting sensitive data.

---

## How It Works

```
You enter a master passphrase + password to store
       ↓
    PassPix generates a random 1920×1080 gradient image
    with soft shapes and natural noise
       ↓
    Your password is encrypted with AES-256-CBC
    (key derived from your master passphrase via PBKDF2-HMAC-SHA256)
       ↓
    The ciphertext + salt + IV + integrity HMAC are
    embedded at pseudo-random pixel positions
       ↓
    Output: a .png file that looks like abstract art
```

To recover the password, you provide the same master passphrase — the correct key is re-derived, and the ciphertext is extracted and decrypted.

---

## Quick Start

### Dependencies

- **C++ compiler** — g++ with C++11 support or newer
- **OpenSSL** — 1.1.0 or newer (3.0+ recommended)
  - Linux: `libssl-dev`, `libcrypto-dev`
  - Windows (MinGW): OpenSSL libraries for MinGW
- **Make** or **CMake** (3.10+)

### Build

```bash
# Linux
sudo apt install build-essential libssl-dev
make

# Windows (MinGW)
mingw32-make
```

Or with CMake:

```bash
mkdir build && cd build
cmake .. && make
```

### Usage

```bash
# Run the program
./passpix

# Follow the interactive menu:
#   1. Encrypt a password → enter master passphrase (twice) + password to store
#   2. Decrypt a password → enter master passphrase + select a file

# Output files are named enc_<random>.png
```

### Testing

```bash
# Run all tests (unit + integration + smoke)
make test

# Or with CMake
cd build && cmake .. && make && ctest
```

---

## Technical Details

| Component | Choice |
|---|---|
| **Encryption** | AES-256-CBC with PKCS#7 padding |
| **Key derivation** | PBKDF2-HMAC-SHA256, 100,000 iterations |
| **Integrity** | HMAC-SHA256 (constant-time verification) |
| **Steganography** | Pseudorandom byte embedding with 2× redundancy + frequency recovery |
| **Image format** | PNG via [LodePNG](https://lodev.org/lodepng/) |
| **Image size** | 1920×1080 RGBA (8-bit per channel) |

### Image embedding layout

Metadata (salt, IV, password hash, HMAC) is stored at multiple redundant locations within the first/last few hundred pixels. The encrypted payload is shuffled across the remaining pixel data using a deterministic seed derived from the key. Redundancy and majority-vote recovery protect against pixel corruption.

---

## Project Structure

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

---

## Security Notes

- **Master passphrase strength matters.** Use a long, unique passphrase — it's the single key to all stored passwords.
- **The generated images are not encrypted containers.** The pixel data looks random but the embedding pattern is deterministic. Security relies on the cryptographic key, not obscurity.
- **This is a password backup/storage tool**, not a general-purpose encryption utility. It is *not* intended to hide the existence of the secret (that's steganography, but the format is known).

---

## License

MIT — see [LICENSE](LICENSE).
