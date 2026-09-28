#include "clipboard.h"
#include <cstdio>
#include <cstdlib>
#include <string>

#ifdef _WIN32
#include <windows.h>
#else
#include <csignal>
#endif

#ifdef _WIN32

namespace {
    bool setClipboardText(const std::string& text) {
        if (!OpenClipboard(nullptr)) return false;
        EmptyClipboard();

        bool ok = true;
        if (!text.empty()) {
            const int wideLen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
            HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, (static_cast<size_t>(wideLen) + 1) * sizeof(wchar_t));
            if (mem == nullptr) {
                CloseClipboard();
                return false;
            }
            wchar_t* buffer = static_cast<wchar_t*>(GlobalLock(mem));
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), buffer, wideLen);
            buffer[wideLen] = L'\0';
            GlobalUnlock(mem);

            // Keep the secret out of clipboard history and cloud clipboard sync
            const UINT excludeFormat = RegisterClipboardFormatW(L"ExcludeClipboardContentFromMonitorProcessing");
            const UINT historyFormat = RegisterClipboardFormatW(L"CanIncludeInClipboardHistory");
            HGLOBAL flag = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
            if (flag != nullptr) {
                *static_cast<DWORD*>(GlobalLock(flag)) = 0;
                GlobalUnlock(flag);
                SetClipboardData(historyFormat, flag);
            }
            HGLOBAL exclude = GlobalAlloc(GMEM_MOVEABLE, sizeof(DWORD));
            if (exclude != nullptr) {
                SetClipboardData(excludeFormat, exclude);
            }

            if (SetClipboardData(CF_UNICODETEXT, mem) == nullptr) {
                GlobalFree(mem);
                ok = false;
            }
        }

        CloseClipboard();
        return ok;
    }
}

bool clipboardAvailable() {
    return true;
}

bool copyToClipboard(const std::string& text) {
    return setClipboardText(text);
}

bool clearClipboard() {
    return setClipboardText("");
}

#else

namespace {
    struct ClipboardTool {
        const char* name;
        const char* copyCommand;
        const char* clearCommand;
        bool needsWayland;
    };

    const ClipboardTool TOOLS[] = {
        {"wl-copy", "wl-copy >/dev/null 2>&1", "wl-copy --clear >/dev/null 2>&1", true},
        {"xclip", "xclip -selection clipboard >/dev/null 2>&1", "xclip -selection clipboard </dev/null >/dev/null 2>&1", false},
        {"xsel", "xsel --clipboard --input >/dev/null 2>&1", "xsel --clipboard --clear >/dev/null 2>&1", false},
    };

    const ClipboardTool* findTool() {
        const bool wayland = std::getenv("WAYLAND_DISPLAY") != nullptr;
        const bool x11 = std::getenv("DISPLAY") != nullptr;
        for (const auto& tool : TOOLS) {
            if (tool.needsWayland ? !wayland : !x11) continue;
            const std::string check = std::string("command -v ") + tool.name + " >/dev/null 2>&1";
            if (std::system(check.c_str()) == 0) return &tool;
        }
        return nullptr;
    }

    // The command is a fixed string; the secret only travels through the pipe.
    bool runWithInput(const char* command, const std::string& input) {
        void (*previous)(int) = std::signal(SIGPIPE, SIG_IGN);
        FILE* pipe = popen(command, "w");
        if (pipe == nullptr) {
            std::signal(SIGPIPE, previous);
            return false;
        }
        const size_t written = input.empty() ? 0 : std::fwrite(input.data(), 1, input.size(), pipe);
        const int status = pclose(pipe);
        std::signal(SIGPIPE, previous);
        return written == input.size() && status == 0;
    }
}

bool clipboardAvailable() {
    return findTool() != nullptr;
}

bool copyToClipboard(const std::string& text) {
    const ClipboardTool* tool = findTool();
    return tool != nullptr && runWithInput(tool->copyCommand, text);
}

bool clearClipboard() {
    const ClipboardTool* tool = findTool();
    return tool != nullptr && runWithInput(tool->clearCommand, "");
}

#endif
