#include "dcmm/antigravity.hpp"

#include "dcmm/path.hpp"
#include "dcmm/walk.hpp"
#include "extension_manifest.hpp"

#include <cstdlib>
#include <utility>
#include <vector>

namespace dcmm {
namespace {

struct RootSpec {
  const char* id;
  const char* label;
  std::string path;
};

std::vector<RootSpec> rootSpecs(const std::string& home) {
  return {
      {"extensions", "Extensions", joinPath(joinPath(home, ".antigravity-ide"), "extensions")},
      {"dot_antigravity_ide", ".antigravity-ide", joinPath(home, ".antigravity-ide")},
      {"shared", ".antigravity-ide-shared", joinPath(home, ".antigravity-ide-shared")},
      {"server", ".antigravity-ide-server", joinPath(home, ".antigravity-ide-server")},
      {"gemini", ".gemini/antigravity-ide", joinPath(joinPath(home, ".gemini"), "antigravity-ide")},
      {"app_support", "Antigravity IDE",
       joinPath(joinPath(home, "Library/Application Support"), "Antigravity IDE")},
  };
}

uint64_t allocatedOrZero(const std::string& path, std::atomic<bool>* cancel,
                         const ProgressFn& progress) {
  if (!pathExists(path)) return 0;
  return directoryAllocatedSize(path, cancel, progress).bytes;
}

}  // namespace

std::string antigravityDisplayName() { return "Antigravity IDE"; }

std::string antigravityAppPath(const std::string& home) {
  const char* leaf = "Antigravity IDE.app";
  std::vector<std::string> roots;
  if (!std::getenv("DCMM_HOME")) roots.push_back("/Applications");
  roots.push_back(joinPath(home, "Applications"));
  for (const auto& root : roots) {
    auto p = joinPath(root, leaf);
    if (pathExists(p)) return p;
  }
  return {};
}

std::string antigravityExtensionsPath(const std::string& home) {
  return rootSpecs(home).front().path;
}

std::vector<std::string> antigravityKnownRoots(const std::string& home) {
  std::vector<std::string> out;
  for (const auto& s : rootSpecs(home)) {
    if (s.id && std::string(s.id) == "extensions") continue;
    out.push_back(s.path);
  }
  return out;
}

std::vector<std::string> antigravityNukePaths(const std::string& home) {
  std::vector<std::string> out;
  auto app = antigravityAppPath(home);
  if (!app.empty()) out.push_back(app);
  for (const auto& p : antigravityKnownRoots(home))
    if (pathExists(p)) out.push_back(p);
  return out;
}

std::vector<AntigravityItem> antigravityItems(const std::string& home, std::atomic<bool>* cancel,
                                              const ProgressFn& progress) {
  std::vector<AntigravityItem> out;
  for (const auto& s : rootSpecs(home)) {
    if (cancel && cancel->load()) break;
    if (!pathExists(s.path)) continue;
    AntigravityItem it;
    it.id = s.id;
    it.label = s.label;
    it.path = s.path;
    it.extensions = std::string(s.id) == "extensions";
    it.bytes = allocatedOrZero(s.path, cancel, progress);
    if (progress) progress(s.path, out.size() + 1, it.bytes);
    out.push_back(std::move(it));
  }
  return out;
}

std::vector<AntigravityExtension> antigravityExtensions(const std::string& home,
                                                        std::atomic<bool>* cancel,
                                                        const ProgressFn& progress) {
  std::vector<AntigravityExtension> out;
  const auto dir = antigravityExtensionsPath(home);
  if (!isDirectory(dir)) return out;
  forEachChild(dir, [&](const std::string& name, const std::string& full, bool isDir) {
    if (cancel && cancel->load()) return;
    if (!isDir) return;
    if (name.empty() || name[0] == '.') return;
    AntigravityExtension ext;
    ext.path = full;
    auto m = extjson::read(full, name);
    ext.name = std::move(m.name);
    ext.version = std::move(m.version);
    ext.publisher = std::move(m.publisher);
    ext.repositoryUrl = std::move(m.repositoryUrl);
    ext.iconPath = std::move(m.iconPath);
    ext.bytes = allocatedOrZero(full, cancel, progress);
    if (progress) progress(full, out.size() + 1, ext.bytes);
    out.push_back(std::move(ext));
  });
  return out;
}

std::vector<AntigravityInstall> antigravityInstalls(const std::string& home,
                                                    std::atomic<bool>* cancel,
                                                    const ProgressFn& progress) {
  std::vector<AntigravityInstall> out;
  auto app = antigravityAppPath(home);
  auto items = antigravityItems(home, cancel, progress);
  if (app.empty() && items.empty()) return out;
  AntigravityInstall inst;
  inst.displayName = antigravityDisplayName();
  inst.appPath = std::move(app);
  inst.items = std::move(items);
  inst.bytes = allocatedOrZero(inst.appPath, cancel, progress);
  for (const auto& it : inst.items) {
    if (it.extensions) continue;
    inst.bytes += it.bytes;
  }
  out.push_back(std::move(inst));
  return out;
}

}  // namespace dcmm
