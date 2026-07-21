#pragma once

#include <vector>
#include <string>

void encryptPassword(const std::string& masterPassphrase, const std::string& password);
std::string decryptPassword(const std::string& masterPassphrase, const std::string& filename);
std::vector<std::string> listEncFiles();
