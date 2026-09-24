#pragma once

#include "dcmm/path.hpp"
#include "dcmm/types.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace dcmm {

struct AntigravityExtension {
  std::string path;
  std::string name;
  std::string iconPath;
  uint64_t bytes = 0;
};

struct AntigravityItem {
  std::string id;
  std::string label;
  std::string path;
  uint64_t bytes = 0;
  bool extensions = false;
};

struct AntigravityInstall {
  std::string displayName;
  std::string appPath;
  std::vector<AntigravityItem> items;
  uint64_t bytes = 0;
};

std::string antigravityDisplayName();
std::string antigravityAppPath(const std::string& home = homeDirectory());
std::string antigravityExtensionsPath(const std::string& home = homeDirectory());
std::vector<std::string> antigravityKnownRoots(const std::string& home = homeDirectory());
std::vector<std::string> antigravityNukePaths(const std::string& home = homeDirectory());

std::vector<AntigravityItem> antigravityItems(const std::string& home = homeDirectory(),
                                              std::atomic<bool>* cancel = nullptr,
                                              const ProgressFn& progress = nullptr);
std::vector<AntigravityExtension> antigravityExtensions(const std::string& home = homeDirectory(),
                                                        std::atomic<bool>* cancel = nullptr,
                                                        const ProgressFn& progress = nullptr);
std::vector<AntigravityInstall> antigravityInstalls(const std::string& home = homeDirectory(),
                                                    std::atomic<bool>* cancel = nullptr,
                                                    const ProgressFn& progress = nullptr);

}  // namespace dcmm
