#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "sha256.hpp"
#include "walk.hpp"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <system_error>
#include <unordered_map>
#include <vector>

namespace dcmm {
namespace fs = std::filesystem;

namespace {

struct HashKey {
  uint8_t d[kSha256Size]{};
  bool operator==(const HashKey& o) const { return std::memcmp(d, o.d, kSha256Size) == 0; }
};

struct HashKeyH {
  std::size_t operator()(const HashKey& k) const {
    std::size_t h = 14695981039346656037ull;
    for (std::size_t i = 0; i < kSha256Size; ++i) {
      h ^= k.d[i];
      h *= 1099511628211ull;
    }
    return h;
  }
};

void collect(const fs::path& dir, uint64_t minBytes, uint64_t maxFiles,
             std::unordered_map<uint64_t, std::vector<std::string>>& bySize, uint64_t& seen,
             std::atomic<bool>* cancel, const ProgressFn& progress) {
  std::error_code ec;
  fs::recursive_directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
  for (; it != fs::recursive_directory_iterator() && !ec; it.increment(ec)) {
    if (cancel && cancel->load()) break;
    if (seen >= maxFiles) break;
    std::error_code lec;
    if (it->is_symlink(lec)) continue;
    if (it->is_directory(lec)) {
      if (skipDirectoryName(it->path().filename().string())) it.disable_recursion_pending();
      continue;
    }
    if (!it->is_regular_file(lec)) continue;
    auto sz = it->file_size(lec);
    if (lec || sz < minBytes) continue;
    bySize[sz].push_back(it->path().string());
    seen++;
    if (progress && (seen % 128 == 0)) progress(it->path().string(), seen, 0);
  }
}

}  // namespace

std::vector<DuplicateGroup> Engine::findDuplicates(const DuplicateOptions& opt,
                                                   const ProgressFn& progress) {
  std::unordered_map<uint64_t, std::vector<std::string>> bySize;
  uint64_t seen = 0;
  for (const auto& root : opt.roots) {
    auto p = expandPath(root);
    if (isDirectory(p)) collect(p, opt.minBytes, opt.maxFiles, bySize, seen, &cancel_, progress);
  }

  std::vector<DuplicateGroup> groups;
  for (auto& [size, files] : bySize) {
    if (cancel_.load()) break;
    if (files.size() < 2) continue;

    std::unordered_map<HashKey, std::vector<std::string>, HashKeyH> prefix;
    for (const auto& f : files) {
      HashKey k;
      if (!sha256File(f, std::min<uint64_t>(size, 64 * 1024), k.d)) continue;
      prefix[k].push_back(f);
    }
    for (auto& [pk, list] : prefix) {
      if (list.size() < 2) continue;
      std::unordered_map<HashKey, std::vector<std::string>, HashKeyH> full;
      for (const auto& f : list) {
        HashKey k;
        if (!sha256File(f, size, k.d)) continue;
        full[k].push_back(f);
        if (progress) progress(f, 0, size);
      }
      for (auto& [hk, same] : full) {
        if (same.size() < 2) continue;
        DuplicateGroup g;
        g.bytes = size;
        g.hashHex = sha256Hex(hk.d);
        for (std::size_t i = 0; i < same.size(); ++i) {
          DuplicateFile df;
          df.path = same[i];
          df.bytes = size;
          df.keep = (i == 0);
          g.files.push_back(std::move(df));
        }
        groups.push_back(std::move(g));
      }
    }
  }
  std::sort(groups.begin(), groups.end(), [](const DuplicateGroup& a, const DuplicateGroup& b) {
    return a.bytes * (a.files.size() - 1) > b.bytes * (b.files.size() - 1);
  });
  return groups;
}

}  // namespace dcmm
