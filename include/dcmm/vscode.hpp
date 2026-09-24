#pragma once

#include "dcmm/path.hpp"
#include "dcmm/types.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace dcmm {

enum class VsCodeEdition { Stable, Insiders };

struct VsCodeExtension {
  std::string path;
  std::string name;
  std::string version;
  std::string publisher;
  std::string repositoryUrl;
  std::string iconPath;
  uint64_t bytes = 0;
};

struct VsCodeItem {
  std::string id;
  std::string label;
  std::string path;
  uint64_t bytes = 0;
  bool extensions = false;
};

struct VsCodeInstall {
  VsCodeEdition edition = VsCodeEdition::Stable;
  std::string displayName;
  std::string appPath;
  std::vector<VsCodeItem> items;
  uint64_t bytes = 0;
};

std::string vsCodeDisplayName(VsCodeEdition edition);
std::string vsCodeAppPath(VsCodeEdition edition, const std::string& home = homeDirectory());
std::string vsCodeExtensionsPath(VsCodeEdition edition, const std::string& home = homeDirectory());
std::vector<std::string> vsCodeKnownRoots(VsCodeEdition edition,
                                          const std::string& home = homeDirectory());
std::vector<std::string> vsCodeNukePaths(VsCodeEdition edition,
                                         const std::string& home = homeDirectory());

std::vector<VsCodeItem> vsCodeItems(VsCodeEdition edition, const std::string& home = homeDirectory(),
                                    std::atomic<bool>* cancel = nullptr,
                                    const ProgressFn& progress = nullptr);
std::vector<VsCodeExtension> vsCodeExtensions(VsCodeEdition edition,
                                              const std::string& home = homeDirectory(),
                                              std::atomic<bool>* cancel = nullptr,
                                              const ProgressFn& progress = nullptr);
std::vector<VsCodeInstall> vsCodeInstalls(const std::string& home = homeDirectory(),
                                          std::atomic<bool>* cancel = nullptr,
                                          const ProgressFn& progress = nullptr);

}  // namespace dcmm
