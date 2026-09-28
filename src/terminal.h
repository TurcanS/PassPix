#pragma once

#include <iostream>
#include <string>

// Selects where prompts and status messages go. Script mode sends them to stderr
// so that stdout carries only the result.
void initTerminal(bool scriptMode);
std::ostream& msg();

bool stdinIsTty();
bool isInteractive();

// All readers return false on end of input.
bool readLine(const std::string& prompt, std::string& out);
bool readHidden(const std::string& prompt, std::string& out);
bool readNumber(const std::string& prompt, long& out);
bool confirm(const std::string& prompt, bool defaultYes);

std::string bold(const std::string& text);
std::string dim(const std::string& text);
std::string success(const std::string& text);
std::string failure(const std::string& text);
std::string warning(const std::string& text);

void eraseLines(int count);
