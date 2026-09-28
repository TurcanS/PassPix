#include "image_utils.h"
#include "crypto_utils.h"
#include "clipboard.h"
#include "terminal.h"
#include "version.h"
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>

namespace {
    constexpr int MAX_ATTEMPTS = 3;
    constexpr size_t RECOMMENDED_PASSPHRASE_LENGTH = 12;

    constexpr int EXIT_OK = 0;
    constexpr int EXIT_ERROR = 1;
    constexpr int EXIT_USAGE = 2;
}

void showBanner();
void showMenu();
void showHelp();
std::string trim(const std::string& text);
bool promptNewPassphrase(std::string& passphrase);
bool promptPasswordToStore(std::string& password);
int encryptFlow(bool scriptMode);
std::string decryptWithRetries(const std::string& path);
void waitAndClearClipboard();
void revealPassword(const std::string& password);
void menuDecrypt();
int runMenu();
int commandDecrypt(const std::string& path, bool copy);
int commandList(const std::string& dir);
int usageError(const std::string& message);

int main(int argc, char* argv[]) {
    initCrypto();
    const std::vector<std::string> args(argv + 1, argv + argc);

    if (args.empty()) {
        initTerminal(false);
        return runMenu();
    }

    initTerminal(true);
    const std::string& command = args[0];

    if (command == "-h" || command == "--help" || command == "help") {
        showHelp();
        return EXIT_OK;
    }

    if (command == "-v" || command == "--version") {
        std::cout << "passpix " << PASSPIX_VERSION << std::endl;
        return EXIT_OK;
    }

    if (command == "encrypt") {
        if (args.size() != 1) return usageError("'encrypt' takes no arguments");
        return encryptFlow(true);
    }

    if (command == "decrypt") {
        std::string path;
        bool copy = false;
        for (size_t i = 1; i < args.size(); i++) {
            if (args[i] == "--copy" || args[i] == "-c") {
                copy = true;
            } else if (path.empty() && (args[i].empty() || args[i][0] != '-')) {
                path = args[i];
            } else {
                return usageError("unexpected argument '" + args[i] + "'");
            }
        }
        if (path.empty()) return usageError("'decrypt' needs an image file");
        return commandDecrypt(path, copy);
    }

    if (command == "list") {
        if (args.size() > 2) return usageError("'list' takes at most one folder");
        return commandList(args.size() == 2 ? args[1] : ".");
    }

    return usageError("unknown command '" + command + "'");
}

int runMenu() {
    showBanner();

    while (true) {
        showMenu();

        std::string choice;
        if (!readLine("Enter your choice: ", choice)) break;
        choice = trim(choice);

        if (choice == "1") {
            encryptFlow(false);
        } else if (choice == "2") {
            menuDecrypt();
        } else if (choice == "3" || choice == "q" || choice == "Q") {
            break;
        } else {
            msg() << failure("Invalid choice.") << std::endl;
        }
    }

    return EXIT_OK;
}

bool promptNewPassphrase(std::string& passphrase) {
    const int attempts = isInteractive() ? MAX_ATTEMPTS : 1;

    for (int attempt = 1; attempt <= attempts; attempt++) {
        if (!readHidden("Enter master passphrase: ", passphrase)) return false;

        if (passphrase.empty()) {
            msg() << failure("Master passphrase cannot be empty.") << std::endl;
            continue;
        }

        std::string confirmation;
        if (!readHidden("Confirm master passphrase: ", confirmation)) {
            wipeString(passphrase);
            return false;
        }

        const bool matches = passphrase == confirmation;
        wipeString(confirmation);
        if (!matches) {
            wipeString(passphrase);
            msg() << failure("Passphrases do not match.") << std::endl;
            continue;
        }

        if (passphrase.size() < RECOMMENDED_PASSPHRASE_LENGTH) {
            msg() << warning("This passphrase is short. " + std::to_string(RECOMMENDED_PASSPHRASE_LENGTH) +
                             "+ characters are recommended; it is the only key to your passwords.") << std::endl;
            if (isInteractive() && !confirm("Use it anyway?", false)) {
                wipeString(passphrase);
                continue;
            }
        }

        return true;
    }

    return false;
}

bool promptPasswordToStore(std::string& password) {
    const int attempts = isInteractive() ? MAX_ATTEMPTS : 1;

    for (int attempt = 1; attempt <= attempts; attempt++) {
        if (!readHidden("Enter password to encrypt: ", password)) return false;

        if (password.empty()) {
            msg() << failure("Password cannot be empty.") << std::endl;
            continue;
        }

        if (password.size() > MAX_PASSWORD_LENGTH) {
            wipeString(password);
            msg() << failure("Password is longer than " + std::to_string(MAX_PASSWORD_LENGTH) + " characters.") << std::endl;
            continue;
        }

        if (isInteractive()) {
            std::string confirmation;
            if (!readHidden("Confirm password: ", confirmation)) {
                wipeString(password);
                return false;
            }
            const bool matches = password == confirmation;
            wipeString(confirmation);
            if (!matches) {
                wipeString(password);
                msg() << failure("Passwords do not match.") << std::endl;
                continue;
            }
        }

        return true;
    }

    return false;
}

int encryptFlow(bool scriptMode) {
    std::string passphrase, password;

    if (!promptNewPassphrase(passphrase)) {
        msg() << failure("Encryption cancelled.") << std::endl;
        return EXIT_ERROR;
    }

    if (!promptPasswordToStore(password)) {
        wipeString(passphrase);
        msg() << failure("Encryption cancelled.") << std::endl;
        return EXIT_ERROR;
    }

    msg() << dim("Deriving key (Argon2id) and generating image...") << std::endl;
    const std::string filename = encryptPassword(passphrase, password);
    wipeString(passphrase);
    wipeString(password);

    if (filename.empty()) {
        msg() << failure("Encryption failed.") << std::endl;
        return EXIT_ERROR;
    }

    msg() << success("Password encrypted to file: " + filename) << std::endl;
    if (scriptMode) {
        std::cout << filename << std::endl;
    }
    return EXIT_OK;
}

std::string decryptWithRetries(const std::string& path) {
    const int attempts = isInteractive() ? MAX_ATTEMPTS : 1;

    for (int attempt = 1; attempt <= attempts; attempt++) {
        std::string passphrase;
        if (!readHidden("Enter master passphrase: ", passphrase)) return "";

        msg() << dim("Deriving key (Argon2id)...") << std::endl;
        std::string password = decryptPassword(passphrase, path);
        wipeString(passphrase);

        if (!password.empty()) return password;

        const int remaining = attempts - attempt;
        if (remaining > 0) {
            msg() << warning("Try again (" + std::to_string(remaining) +
                             (remaining == 1 ? " attempt" : " attempts") + " left).") << std::endl;
        }
    }

    msg() << failure("Could not decrypt " + path + ".") << std::endl;
    return "";
}

void waitAndClearClipboard() {
    std::string ignored;
    readLine(success("Copied to clipboard.") + " Press Enter to clear it...", ignored);
    if (clearClipboard()) {
        msg() << success("Clipboard cleared.") << std::endl;
    } else {
        msg() << warning("Could not clear the clipboard.") << std::endl;
    }
}

void revealPassword(const std::string& password) {
    if (!isInteractive()) {
        msg() << "Decrypted password: " << password << std::endl;
        return;
    }

    if (clipboardAvailable() && confirm("Copy to clipboard instead of showing it?", true)) {
        if (copyToClipboard(password)) {
            waitAndClearClipboard();
            return;
        }
        msg() << warning("Could not copy to the clipboard; showing it instead.") << std::endl;
    }

    msg() << "Decrypted password: " << bold(password) << std::endl;
    std::string ignored;
    readLine(dim("Press Enter to hide it..."), ignored);
    eraseLines(2);
    msg() << success("Password hidden.") << std::endl;
}

void menuDecrypt() {
    const auto files = listEncFiles();
    if (files.empty()) {
        msg() << warning("No PassPix files found in the current folder.") << std::endl;
        return;
    }

    msg() << bold("Select a file to decrypt:") << std::endl;
    for (size_t i = 0; i < files.size(); i++) {
        msg() << "  " << (i + 1) << ". " << files[i] << std::endl;
    }
    msg() << dim("  0. Back") << std::endl;

    long fileIndex = 0;
    if (!readNumber("Enter file number: ", fileIndex) || fileIndex == 0) return;

    if (fileIndex < 1 || static_cast<size_t>(fileIndex) > files.size()) {
        msg() << failure("Invalid selection.") << std::endl;
        return;
    }

    std::string password = decryptWithRetries(files[static_cast<size_t>(fileIndex) - 1]);
    if (password.empty()) return;

    revealPassword(password);
    wipeString(password);
}

int commandDecrypt(const std::string& path, bool copy) {
    std::error_code ec;
    if (!std::filesystem::is_regular_file(path, ec)) {
        msg() << failure("File not found: " + path) << std::endl;
        return EXIT_ERROR;
    }

    std::string password = decryptWithRetries(path);
    if (password.empty()) return EXIT_ERROR;

    int status = EXIT_OK;
    if (copy) {
        if (!copyToClipboard(password)) {
            msg() << failure("No clipboard available (install wl-clipboard, xclip or xsel).") << std::endl;
            status = EXIT_ERROR;
        } else if (stdinIsTty()) {
            waitAndClearClipboard();
        } else {
            msg() << warning("Copied to clipboard. It will not be cleared automatically.") << std::endl;
        }
    } else {
        std::cout << password << std::endl;
    }

    wipeString(password);
    return status;
}

int commandList(const std::string& dir) {
    std::error_code ec;
    if (!std::filesystem::is_directory(dir, ec)) {
        msg() << failure("Not a folder: " + dir) << std::endl;
        return EXIT_ERROR;
    }

    const auto files = listEncFiles(dir);
    if (files.empty()) {
        msg() << dim("No PassPix files found in " + dir + ".") << std::endl;
    }
    for (const auto& file : files) {
        std::cout << (dir == "." ? file : (std::filesystem::path(dir) / file).string()) << std::endl;
    }
    return EXIT_OK;
}

int usageError(const std::string& message) {
    std::cerr << "passpix: " << message << std::endl;
    std::cerr << "Run 'passpix --help' for usage." << std::endl;
    return EXIT_USAGE;
}

std::string trim(const std::string& text) {
    const auto start = text.find_first_not_of(" \t");
    if (start == std::string::npos) return "";
    const auto end = text.find_last_not_of(" \t");
    return text.substr(start, end - start + 1);
}

void showBanner() {
    msg() << bold("PassPix") << " " << PASSPIX_VERSION << dim("  hide encrypted passwords in images") << std::endl;
    msg() << std::string(55, '=') << std::endl;
}

void showMenu() {
    msg() << "\n" << bold("=== PassPix Menu ===") << std::endl;
    msg() << "1. Encrypt Password" << std::endl;
    msg() << "2. Decrypt Password" << std::endl;
    msg() << "3. Exit" << std::endl;
}

void showHelp() {
    std::cout << "PassPix " << PASSPIX_VERSION << " - hide encrypted passwords inside PNG images\n"
              << "\n"
              << "Usage:\n"
              << "  passpix                           Start the interactive menu\n"
              << "  passpix encrypt                   Encrypt a password into a new image and print its name\n"
              << "  passpix decrypt <image> [--copy]  Print the stored password, or copy it to the clipboard\n"
              << "  passpix list [folder]             List PassPix images (default: current folder)\n"
              << "  passpix -h, --help                Show this help\n"
              << "  passpix -v, --version             Show the version\n"
              << "\n"
              << "Passphrases are read from the terminal with echo off (or from stdin when piped),\n"
              << "never from command-line arguments.\n"
              << "\n"
              << "Exit codes: 0 success, 1 failure, 2 usage error." << std::endl;
}
