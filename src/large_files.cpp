#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "walk.hpp"

#include <algorithm>
#include <filesystem>
#include <queue>
#include <system_error>

namespace dcmm {
namespace fs = std::filesystem;

std::vector<LargeFile> Engine::findLargeFiles(const LargeFileOptions& opt,
                                              const ProgressFn& progress) {
  struct Node {
    uint64_t bytes = 0;
    LargeFile file;
    bool operator<(const Node& o) const { return bytes > o.bytes; }
  };
  std::priority_queue<Node> heap;
  uint64_t visited = 0;
  const std::string home = homeDirectory();

  for (const auto& root : opt.roots) {
    auto p = expandPath(root);
    std::error_code ec;
    auto resolved = fs::weakly_canonical(p, ec);
    if (!ec && !resolved.empty()) p = resolved.string();
    if (!fs::is_directory(p, ec)) continue;
    fs::recursive_directory_iterator it(p, fs::directory_options::skip_permission_denied, ec);
    for (; it != fs::recursive_directory_iterator() && !ec; it.increment(ec)) {
      if (cancel_.load()) break;
      std::error_code lec;
      if (it->is_symlink(lec)) continue;
      if (it->is_directory(lec)) {
        auto name = it->path().filename().string();
        if (skipDirectoryName(name) || (name == "Library" && p == home)) {
          it.disable_recursion_pending();
        }
        continue;
      }
      if (!it->is_regular_file(lec)) continue;
      auto sz = it->file_size(lec);
      visited++;
      if (progress && (visited % 256 == 0)) progress(it->path().string(), visited, sz);
      if (lec || sz < opt.minBytes) continue;
      LargeFile lf;
      lf.path = it->path().string();
      lf.bytes = sz;
      auto ftime = it->last_write_time(lec);
      (void)ftime;
      lf.mtime = 0;
      lf.selected = false;
      heap.push(Node{sz, std::move(lf)});
      if (heap.size() > opt.limit) heap.pop();
    }
  }

  std::vector<LargeFile> out;
  out.reserve(heap.size());
  while (!heap.empty()) {
    out.push_back(heap.top().file);
    heap.pop();
  }
  std::reverse(out.begin(), out.end());
  return out;
}

}  // namespace dcmm
