#include "dcmm/path.hpp"

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <system_error>

#if !defined(_WIN32)
#include <limits.h>
#include <stdlib.h>
#endif

namespace dcmm {
namespace fs = std::filesystem;

std::string formatBytes(uint64_t bytes) {
  static const char* units[] = {"B", "KB", "MB", "GB", "TB", "PB"};
  double v = static_cast<double>(bytes);
  int i = 0;
  while (v >= 1024.0 && i < 5) {
    v /= 1024.0;
    ++i;
  }
  char buf[64];
  if (i == 0)
    std::snprintf(buf, sizeof(buf), "%llu B", static_cast<unsigned long long>(bytes));
  else if (v >= 100.0)
    std::snprintf(buf, sizeof(buf), "%.0f %s", v, units[i]);
  else
    std::snprintf(buf, sizeof(buf), "%.1f %s", v, units[i]);
  return buf;
}

std::string formatCount(uint64_t n, const char* singular, const char* plural) {
  char buf[128];
  std::snprintf(buf, sizeof(buf), "%llu %s", static_cast<unsigned long long>(n),
                n == 1 ? singular : plural);
  return buf;
}

std::string homeDirectory() {
  if (const char* o = std::getenv("DCMM_HOME"); o && *o) return o;
#ifdef _WIN32
  if (const char* u = std::getenv("USERPROFILE"); u && *u) return u;
#else
  if (const char* h = std::getenv("HOME"); h && *h) return h;
#endif
  return fs::current_path().string();
}

std::string expandPath(const std::string& path) {
  if (path.empty()) return path;
  if (path[0] == '~') {
    if (path.size() == 1) return homeDirectory();
    if (path[1] == '/' || path[1] == '\\') return joinPath(homeDirectory(), path.substr(2));
  }
  return path;
}

std::string displayName(const std::string& path) {
  return fs::path(path).filename().string();
}

std::string parentPath(const std::string& path) {
  return fs::path(path).parent_path().string();
}

std::string joinPath(const std::string& a, const std::string& b) {
  if (a.empty()) return b;
  if (b.empty()) return a;
  return (fs::path(a) / b).string();
}

std::string nativeSeparators(const std::string& path) {
  return fs::path(path).make_preferred().string();
}

bool pathExists(const std::string& path) {
  std::error_code ec;
  return fs::exists(expandPath(path), ec);
}

bool isDirectory(const std::string& path) {
  std::error_code ec;
  return fs::is_directory(expandPath(path), ec);
}

bool isRegularFile(const std::string& path) {
  std::error_code ec;
  return fs::is_regular_file(expandPath(path), ec);
}

std::string resolveRealPath(const std::string& path) {
  std::error_code ec;
  auto p = fs::weakly_canonical(expandPath(path), ec);
  if (ec) return expandPath(path);
  auto s = p.string();
  while (s.size() > 1 && (s.back() == '/' || s.back() == '\\')) s.pop_back();
  return s;
}

}  // namespace dcmm
