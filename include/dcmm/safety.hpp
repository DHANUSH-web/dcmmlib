#pragma once

#include <string>

namespace dcmm {

bool isProtectedPath(const std::string& path);
bool isSafeToTrash(const std::string& path);
bool isBrowserCacheName(const std::string& name);
bool isSensitiveFileName(const std::string& name);

}  // namespace dcmm
