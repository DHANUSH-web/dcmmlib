#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace dcmm {

std::string formatBytes(uint64_t bytes);
std::string formatCount(uint64_t n, const char* singular, const char* plural);

/// Home directory. Honors DCMM_HOME (test override), else HOME / USERPROFILE.
std::string homeDirectory();

std::string expandPath(const std::string& path);
std::string displayName(const std::string& path);
std::string parentPath(const std::string& path);
std::string joinPath(const std::string& a, const std::string& b);
std::string nativeSeparators(const std::string& path);

bool pathExists(const std::string& path);
bool isDirectory(const std::string& path);
bool isRegularFile(const std::string& path);
std::string resolveRealPath(const std::string& path);

}  // namespace dcmm
