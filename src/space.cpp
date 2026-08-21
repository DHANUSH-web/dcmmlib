#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "walk.hpp"

#include <algorithm>

namespace dcmm {

std::vector<SpaceNode> Engine::spaceLens(const ProgressFn& progress) {
  std::vector<SpaceNode> nodes;
  const std::string home = homeDirectory();
  auto add = [&](const std::string& name, const std::string& full, bool isDir) {
    if (cancel_.load()) return;
    auto sc = directoryAllocatedSize(full, &cancel_, progress);
    if (sc.bytes == 0) return;
    SpaceNode n;
    n.path = full;
    n.name = name;
    n.bytes = sc.bytes;
    n.isDir = isDir;
    nodes.push_back(std::move(n));
  };

  forEachChild(home, [&](const std::string& name, const std::string& full, bool isDir) {
    if (name == ".Trash" || name == ".local") return;
    // List Library children instead of Library as one blob so we do not
    // show Library and Library/Containers as overlapping 400+ GB rows.
    if (name == "Library" && isDir) {
      forEachChild(full, [&](const std::string& child, const std::string& childPath, bool childDir) {
        add(std::string("Library/") + child, childPath, childDir);
      });
      return;
    }
    add(name, full, isDir);
  });

  std::sort(nodes.begin(), nodes.end(), [](const SpaceNode& a, const SpaceNode& b) {
    if (a.bytes != b.bytes) return a.bytes > b.bytes;
    return a.name < b.name;
  });
  if (nodes.size() > 40) nodes.resize(40);
  return nodes;
}

}  // namespace dcmm
