#include "dcmm/vscode.hpp"

#include "dcmm/path.hpp"
#include "dcmm/walk.hpp"

#include <cstdlib>
#include <fstream>
#include <utility>
#include <vector>

namespace dcmm {
namespace {

struct RootSpec {
  const char* id;
  const char* label;
  std::string path;
};

std::string jsonStringField(const std::string& body, const char* key) {
  const std::string pat = std::string("\"") + key + "\"";
  auto pos = body.find(pat);
  if (pos == std::string::npos) return {};
  pos = body.find(':', pos + pat.size());
  if (pos == std::string::npos) return {};
  pos = body.find('"', pos + 1);
  if (pos == std::string::npos) return {};
  ++pos;
  std::string out;
  for (; pos < body.size(); ++pos) {
    char c = body[pos];
    if (c == '"') break;
    if (c == '\\' && pos + 1 < body.size()) {
      out.push_back(body[++pos]);
      continue;
    }
    out.push_back(c);
  }
  return out;
}

std::string readSmallFile(const std::string& path) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::string s(64 * 1024, '\0');
  in.read(s.data(), static_cast<std::streamsize>(s.size()));
  s.resize(static_cast<std::size_t>(in.gcount()));
  return s;
}

std::string extensionIcon(const std::string& dir) {
  const char* names[] = {"icon.png", "logo.png", nullptr};
  for (int i = 0; names[i]; ++i) {
    auto p = joinPath(dir, names[i]);
    if (isRegularFile(p)) return p;
  }
  return {};
}

std::vector<RootSpec> rootSpecs(VsCodeEdition edition, const std::string& home) {
  const bool insiders = edition == VsCodeEdition::Insiders;
  const char* dot = insiders ? ".vscode-insiders" : ".vscode";
  const char* shared = insiders ? ".vscode-insiders-shared" : ".vscode-shared";
  const char* server = insiders ? ".vscode-insiders-server" : ".vscode-server";
  const char* support = insiders ? "Code - Insiders" : "Code";
  return {
      {"extensions", "Extensions", joinPath(joinPath(home, dot), "extensions")},
      {"dot_vscode", dot, joinPath(home, dot)},
      {"shared", shared, joinPath(home, shared)},
      {"server", server, joinPath(home, server)},
      {"app_support", support, joinPath(joinPath(home, "Library/Application Support"), support)},
  };
}

uint64_t allocatedOrZero(const std::string& path, std::atomic<bool>* cancel,
                         const ProgressFn& progress) {
  if (!pathExists(path)) return 0;
  return directoryAllocatedSize(path, cancel, progress).bytes;
}

}  // namespace

std::string vsCodeDisplayName(VsCodeEdition edition) {
  return edition == VsCodeEdition::Insiders ? "Visual Studio Code - Insiders"
                                            : "Visual Studio Code";
}

std::string vsCodeAppPath(VsCodeEdition edition, const std::string& home) {
  const char* leaf = edition == VsCodeEdition::Insiders ? "Visual Studio Code - Insiders.app"
                                                        : "Visual Studio Code.app";
  std::vector<std::string> roots;
  if (!std::getenv("DCMM_HOME")) roots.push_back("/Applications");
  roots.push_back(joinPath(home, "Applications"));
  for (const auto& root : roots) {
    auto p = joinPath(root, leaf);
    if (pathExists(p)) return p;
  }
  return {};
}

std::string vsCodeExtensionsPath(VsCodeEdition edition, const std::string& home) {
  return rootSpecs(edition, home).front().path;
}

std::vector<std::string> vsCodeKnownRoots(VsCodeEdition edition, const std::string& home) {
  std::vector<std::string> out;
  for (const auto& s : rootSpecs(edition, home)) {
    if (s.id && std::string(s.id) == "extensions") continue;
    out.push_back(s.path);
  }
  return out;
}

std::vector<std::string> vsCodeNukePaths(VsCodeEdition edition, const std::string& home) {
  std::vector<std::string> out;
  auto app = vsCodeAppPath(edition, home);
  if (!app.empty()) out.push_back(app);
  for (const auto& p : vsCodeKnownRoots(edition, home))
    if (pathExists(p)) out.push_back(p);
  return out;
}

std::vector<VsCodeItem> vsCodeItems(VsCodeEdition edition, const std::string& home,
                                    std::atomic<bool>* cancel, const ProgressFn& progress) {
  std::vector<VsCodeItem> out;
  for (const auto& s : rootSpecs(edition, home)) {
    if (cancel && cancel->load()) break;
    if (!pathExists(s.path)) continue;
    VsCodeItem it;
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

std::vector<VsCodeExtension> vsCodeExtensions(VsCodeEdition edition, const std::string& home,
                                              std::atomic<bool>* cancel,
                                              const ProgressFn& progress) {
  std::vector<VsCodeExtension> out;
  const auto dir = vsCodeExtensionsPath(edition, home);
  if (!isDirectory(dir)) return out;
  forEachChild(dir, [&](const std::string& name, const std::string& full, bool isDir) {
    if (cancel && cancel->load()) return;
    if (!isDir) return;
    if (name.empty() || name[0] == '.') return;
    VsCodeExtension ext;
    ext.path = full;
    ext.iconPath = extensionIcon(full);
    auto json = readSmallFile(joinPath(full, "package.json"));
    ext.name = jsonStringField(json, "displayName");
    if (ext.name.empty() || (ext.name.size() >= 2 && ext.name.front() == '%' && ext.name.back() == '%'))
      ext.name = name;
    ext.bytes = allocatedOrZero(full, cancel, progress);
    if (progress) progress(full, out.size() + 1, ext.bytes);
    out.push_back(std::move(ext));
  });
  return out;
}

std::vector<VsCodeInstall> vsCodeInstalls(const std::string& home, std::atomic<bool>* cancel,
                                          const ProgressFn& progress) {
  std::vector<VsCodeInstall> out;
  const VsCodeEdition eds[] = {VsCodeEdition::Stable, VsCodeEdition::Insiders};
  for (auto ed : eds) {
    if (cancel && cancel->load()) break;
    auto app = vsCodeAppPath(ed, home);
    auto items = vsCodeItems(ed, home, cancel, progress);
    if (app.empty() && items.empty()) continue;
    VsCodeInstall inst;
    inst.edition = ed;
    inst.displayName = vsCodeDisplayName(ed);
    inst.appPath = std::move(app);
    inst.items = std::move(items);
    inst.bytes = allocatedOrZero(inst.appPath, cancel, progress);
    for (const auto& it : inst.items) {
      if (it.extensions) continue;
      inst.bytes += it.bytes;
    }
    out.push_back(std::move(inst));
  }
  return out;
}

}  // namespace dcmm
