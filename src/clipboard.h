#pragma once

#include <string>

bool clipboardAvailable();
bool copyToClipboard(const std::string& text);
bool clearClipboard();
