#include "walk.hpp"

#include "dcmm/path.hpp"

#include <filesystem>
#include <system_error>

namespace dcmm {
namespace fs = std::filesystem;

bool skipDirectoryName(const std::string& name) {
  return name == ".git" || name == ".svn" || name == ".hg" || name == ".Trash" ||
         name == "node_modules" || name == "Pods" || name == ".build" || name == "Carthage" ||
         name == ".photoslibrary" || name == ".fseventsd" || name == ".Spotlight-V100" ||
         name == ".DocumentRevisions-V100" || name == ".TemporaryItems" || name == ".MobileBackups";
}

SizeCount directorySize(const std::string& path, std::atomic<bool>* cancel,
                        const ProgressFn& progress) {
  SizeCount out;
  std::error_code ec;
  fs::path root(path);
  auto st = fs::symlink_status(root, ec);
  if (ec) return out;
  if (fs::is_symlink(st)) return out;
  if (fs::is_regular_file(st)) {
    out.bytes = fs::file_size(root, ec);
    out.files = 1;
    return out;
  }
  if (!fs::is_directory(st)) return out;

  const auto opts = fs::directory_options::skip_permission_denied;
  for (fs::recursive_directory_iterator it(root, opts, ec), end; it != end && !ec; it.increment(ec)) {
    if (cancel && cancel->load()) break;
    const auto& entry = *it;
    std::error_code lec;
    if (entry.is_symlink(lec)) continue;
    if (entry.is_directory(lec)) {
      auto name = entry.path().filename().string();
      if (name == ".git" || name == ".svn") {
        it.disable_recursion_pending();
        continue;
      }
      if (progress && (out.files % 64 == 0)) progress(entry.path().string(), out.files, out.bytes);
      continue;
    }
    if (entry.is_regular_file(lec)) {
      auto sz = entry.file_size(lec);
      if (!lec) {
        out.bytes += sz;
        out.files += 1;
      }
    }
  }
  return out;
}

void forEachChild(const std::string& dir,
                  const std::function<void(const std::string& name, const std::string& full,
                                           bool isDir)>& fn) {
  std::error_code ec;
  fs::directory_iterator it(dir, fs::directory_options::skip_permission_denied, ec);
  if (ec) return;
  for (; it != fs::directory_iterator() && !ec; it.increment(ec)) {
    std::error_code lec;
    auto name = it->path().filename().string();
    bool isDir = it->is_directory(lec);
    fn(name, it->path().string(), isDir);
  }
}

}  // namespace dcmm
