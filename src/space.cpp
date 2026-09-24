#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "dcmm/walk.hpp"

#include <algorithm>
#include <atomic>
#include <functional>
#include <thread>
#include <vector>

namespace dcmm {
namespace {

unsigned spaceLensWorkers(std::size_t jobs) {
  unsigned n = std::thread::hardware_concurrency();
  if (n == 0) n = 4;
  if (n > 8) n = 8;
  if (jobs == 0) return 1;
  if (n > jobs) n = static_cast<unsigned>(jobs);
  return n;
}

void parallelIndex(std::size_t count, unsigned workers, const std::function<void(std::size_t)>& fn) {
  if (count == 0) return;
  std::atomic<std::size_t> next{0};
  auto worker = [&] {
    for (;;) {
      std::size_t i = next.fetch_add(1);
      if (i >= count) return;
      fn(i);
    }
  };
  std::vector<std::thread> pool;
  pool.reserve(workers > 0 ? workers - 1 : 0);
  for (unsigned t = 1; t < workers; ++t) pool.emplace_back(worker);
  worker();
  for (auto& t : pool) t.join();
}

}  // namespace

std::vector<SpaceNode> Engine::spaceLens(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  struct Job {
    std::string name;
    std::string path;
    bool isDir = true;
  };
  std::vector<Job> jobs;
  const std::string home = homeDirectory();
  forEachChild(home, [&](const std::string& name, const std::string& full, bool isDir) {
    if (name == ".Trash" || name == ".local") return;
    if (name == "Library" && isDir) {
      forEachChild(full, [&](const std::string& child, const std::string& childPath, bool childDir) {
        jobs.push_back({std::string("Library/") + child, childPath, childDir});
      });
      return;
    }
    jobs.push_back({name, full, isDir});
  });

  std::vector<SpaceNode> nodes(jobs.size());
  parallelIndex(jobs.size(), spaceLensWorkers(jobs.size()), [&](std::size_t i) {
    if (cancel_.load()) return;
    auto sc = directoryAllocatedSizeAll(jobs[i].path, &cancel_, progress);
    SpaceNode n;
    n.path = jobs[i].path;
    n.name = jobs[i].name;
    n.bytes = sc.bytes;
    n.isDir = jobs[i].isDir;
    nodes[i] = std::move(n);
  });

  nodes.erase(std::remove_if(nodes.begin(), nodes.end(),
                             [](const SpaceNode& n) { return n.bytes == 0 || n.path.empty(); }),
              nodes.end());
  std::sort(nodes.begin(), nodes.end(), [](const SpaceNode& a, const SpaceNode& b) {
    if (a.bytes != b.bytes) return a.bytes > b.bytes;
    return a.name < b.name;
  });
  if (nodes.size() > 40) nodes.resize(40);
  return nodes;
}

std::vector<SpaceNode> Engine::spaceLensChildren(const std::string& dir, const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  if (dir.empty()) return {};
  auto m = spaceLensMeasure(dir, &cancel_, progress);
  if (m.children.size() > 80) m.children.resize(80);
  return std::move(m.children);
}

}  // namespace dcmm
