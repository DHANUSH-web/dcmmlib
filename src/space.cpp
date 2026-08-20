#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "walk.hpp"

#include <algorithm>

namespace dcmm {

std::vector<SpaceNode> Engine::spaceLens(const ProgressFn& progress) {
  std::vector<SpaceNode> nodes;
  const std::string home = homeDirectory();
  forEachChild(home, [&](const std::string& name, const std::string& full, bool isDir) {
    if (cancel_.load()) return;
    if (name == ".Trash" || name == ".local") return;
    auto sc = directorySize(full, &cancel_, progress);
    if (sc.bytes == 0) return;
    SpaceNode n;
    n.path = full;
    n.name = name;
    n.bytes = sc.bytes;
    n.isDir = isDir;
    nodes.push_back(std::move(n));
  });

#if defined(__APPLE__)
  const std::string lib = joinPath(home, "Library");
  static const char* heavy[] = {"Caches", "Application Support", "Containers", "Group Containers",
                                "Developer", "Logs", nullptr};
  for (int i = 0; heavy[i]; ++i) {
    if (cancel_.load()) break;
    auto p = joinPath(lib, heavy[i]);
    if (!pathExists(p)) continue;
    auto sc = directorySize(p, &cancel_, progress);
    if (!sc.bytes) continue;
    SpaceNode n;
    n.path = p;
    n.name = std::string("Library/") + heavy[i];
    n.bytes = sc.bytes;
    n.isDir = true;
    nodes.push_back(std::move(n));
  }
#endif

  std::sort(nodes.begin(), nodes.end(), [](const SpaceNode& a, const SpaceNode& b) {
    if (a.bytes != b.bytes) return a.bytes > b.bytes;
    return a.name < b.name;
  });
  if (nodes.size() > 40) nodes.resize(40);
  return nodes;
}

}  // namespace dcmm
