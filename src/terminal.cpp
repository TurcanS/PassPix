#include "terminal.h"
#include <cstdlib>
#include <csignal>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

namespace {
    bool useStderr = false;
    bool colorEnabled = false;

#ifdef _WIN32
    HANDLE inputHandle = INVALID_HANDLE_VALUE;
    DWORD savedMode = 0;
    volatile bool echoDisabled = false;

    BOOL WINAPI restoreOnCtrl(DWORD) {
        if (echoDisabled) SetConsoleMode(inputHandle, savedMode);
        return FALSE;
    }
#else
    struct termios savedTermios;
    volatile sig_atomic_t echoDisabled = 0;

    void restoreOnSignal(int sig) {
        if (echoDisabled) tcsetattr(STDIN_FILENO, TCSANOW, &savedTermios);
        std::signal(sig, SIG_DFL);
        std::raise(sig);
    }
#endif

    bool fdIsTty(int fd) {
#ifdef _WIN32
        return _isatty(fd) != 0;
#else
        return isatty(fd) != 0;
#endif
    }

    std::string colorize(const std::string& code, const std::string& text) {
        if (!colorEnabled) return text;
        return "\033[" + code + "m" + text + "\033[0m";
    }

    void setEcho(bool enabled) {
#ifdef _WIN32
        if (enabled) {
            SetConsoleMode(inputHandle, savedMode);
            echoDisabled = false;
        } else {
            GetConsoleMode(inputHandle, &savedMode);
            echoDisabled = true;
            SetConsoleMode(inputHandle, savedMode & ~static_cast<DWORD>(ENABLE_ECHO_INPUT));
        }
#else
        if (enabled) {
            tcsetattr(STDIN_FILENO, TCSANOW, &savedTermios);
            echoDisabled = 0;
        } else {
            tcgetattr(STDIN_FILENO, &savedTermios);
            struct termios hidden = savedTermios;
            hidden.c_lflag &= ~static_cast<tcflag_t>(ECHO);
            echoDisabled = 1;
            tcsetattr(STDIN_FILENO, TCSANOW, &hidden);
        }
#endif
    }
}

void initTerminal(bool scriptMode) {
    useStderr = scriptMode;
    const int fd = scriptMode ? 2 : 1;
    colorEnabled = fdIsTty(fd) && std::getenv("NO_COLOR") == nullptr;

#ifdef _WIN32
    inputHandle = GetStdHandle(STD_INPUT_HANDLE);
    SetConsoleCtrlHandler(restoreOnCtrl, TRUE);
    SetConsoleOutputCP(CP_UTF8);
    if (colorEnabled) {
        HANDLE out = GetStdHandle(scriptMode ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
        DWORD mode = 0;
        if (!GetConsoleMode(out, &mode) ||
            !SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING)) {
            colorEnabled = false;
        }
    }
#else
    std::signal(SIGINT, restoreOnSignal);
    std::signal(SIGTERM, restoreOnSignal);
#endif
}

std::ostream& msg() {
    return useStderr ? std::cerr : std::cout;
}

bool stdinIsTty() {
    return fdIsTty(0);
}

bool isInteractive() {
    return stdinIsTty() && fdIsTty(useStderr ? 2 : 1);
}

bool readLine(const std::string& prompt, std::string& out) {
    msg() << prompt << std::flush;
    if (!std::getline(std::cin, out)) {
        msg() << std::endl;
        return false;
    }
    if (!out.empty() && out.back() == '\r') out.pop_back();
    return true;
}

bool readHidden(const std::string& prompt, std::string& out) {
    if (!stdinIsTty()) return readLine(prompt, out);

    msg() << prompt << std::flush;
    setEcho(false);
    const bool ok = static_cast<bool>(std::getline(std::cin, out));
    setEcho(true);
    msg() << std::endl;
    if (ok && !out.empty() && out.back() == '\r') out.pop_back();
    return ok;
}

bool readNumber(const std::string& prompt, long& out) {
    std::string line;
    while (readLine(prompt, line)) {
        char* end = nullptr;
        const long value = std::strtol(line.c_str(), &end, 10);
        if (!line.empty() && end != line.c_str() && *end == '\0') {
            out = value;
            return true;
        }
        msg() << failure("Please enter a number.") << std::endl;
    }
    return false;
}

bool confirm(const std::string& prompt, bool defaultYes) {
    std::string line;
    if (!readLine(prompt + (defaultYes ? " [Y/n] " : " [y/N] "), line)) return false;
    if (line.empty()) return defaultYes;
    return line[0] == 'y' || line[0] == 'Y';
}

std::string bold(const std::string& text) {
    return colorize("1", text);
}

std::string dim(const std::string& text) {
    return colorize("2", text);
}

std::string success(const std::string& text) {
    return colorize("32", colorEnabled ? "✓ " + text : text);
}

std::string failure(const std::string& text) {
    return colorize("31", colorEnabled ? "✗ " + text : text);
}

std::string warning(const std::string& text) {
    return colorize("33", colorEnabled ? "! " + text : text);
}

void eraseLines(int count) {
    if (!colorEnabled) return;
    for (int i = 0; i < count; i++) {
        msg() << "\033[1A\033[2K";
    }
    msg() << std::flush;
}
