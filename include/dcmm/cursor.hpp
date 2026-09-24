#pragma once

#include "dcmm/path.hpp"
#include "dcmm/types.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace dcmm {

struct CursorExtension {
  std::string path;
  std::string name;
  std::string version;
  std::string publisher;
  std::string repositoryUrl;
  std::string iconPath;
  uint64_t bytes = 0;
};

struct CursorItem {
  std::string id;
  std::string label;
  std::string path;
  uint64_t bytes = 0;
  bool extensions = false;
};

struct CursorInstall {
  std::string displayName;
  std::string appPath;
  std::vector<CursorItem> items;
  uint64_t bytes = 0;
};

std::string cursorDisplayName();
std::string cursorAppPath(const std::string& home = homeDirectory());
std::string cursorExtensionsPath(const std::string& home = homeDirectory());
std::vector<std::string> cursorKnownRoots(const std::string& home = homeDirectory());
std::vector<std::string> cursorNukePaths(const std::string& home = homeDirectory());

std::vector<CursorItem> cursorItems(const std::string& home = homeDirectory(),
                                    std::atomic<bool>* cancel = nullptr,
                                    const ProgressFn& progress = nullptr);
std::vector<CursorExtension> cursorExtensions(const std::string& home = homeDirectory(),
                                              std::atomic<bool>* cancel = nullptr,
                                              const ProgressFn& progress = nullptr);
std::vector<CursorInstall> cursorInstalls(const std::string& home = homeDirectory(),
                                          std::atomic<bool>* cancel = nullptr,
                                          const ProgressFn& progress = nullptr);

}  // namespace dcmm
