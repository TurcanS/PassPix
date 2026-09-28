#pragma once

#include <vector>
#include <string>

const size_t MAX_PASSWORD_LENGTH = 1024;

// Returns the name of the written image, or an empty string on failure.
std::string encryptPassword(const std::string& masterPassphrase, const std::string& password);
// Returns the stored password, or an empty string on failure.
std::string decryptPassword(const std::string& masterPassphrase, const std::string& filename);
// Returns the sorted names of the PassPix images in dir.
std::vector<std::string> listEncFiles(const std::string& dir = ".");
