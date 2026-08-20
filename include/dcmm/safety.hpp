#pragma once

#include <string>

namespace dcmm {

bool isProtectedPath(const std::string& path);
bool isSafeToTrash(const std::string& path);
bool isBrowserCacheName(const std::string& name);
bool isSensitiveFileName(const std::string& name);
bool isOwnedByCurrentUser(const std::string& path);
bool isJunkCategoryRoot(const std::string& path);

}  // namespace dcmm
