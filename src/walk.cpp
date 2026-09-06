#include "dcmm/walk.hpp"

#include "dcmm/path.hpp"

#include <cerrno>
#include <filesystem>
#include <system_error>
#include <vector>

#if !defined(_WIN32)
#include <sys/stat.h>
#endif

namespace dcmm {
namespace fs = std::filesystem;

namespace {

uint64_t allocatedBytes(const fs::path& p, std::error_code& ec) {
#if defined(_WIN32)
  auto n = fs::file_size(p, ec);
  return ec ? 0 : n;
#else
  struct stat st {};
  if (lstat(p.c_str(), &st) != 0) {
    ec = std::error_code(errno, std::generic_category());
    return 0;
  }
  ec.clear();
  if ((st.st_mode & S_IFMT) == S_IFLNK) return 0;
  return static_cast<uint64_t>(st.st_blocks) * 512ull;
#endif
}

bool skipGitSvn(const std::string& name) { return name == ".git" || name == ".svn"; }

/// One failed subdirectory must not abort the rest of the tree. recursive_directory_iterator
/// stops the whole walk when increment() sets an error, even with skip_permission_denied.
template <typename SkipDir, typename OnFile>
void walkRegularFiles(const fs::path& root, std::atomic<bool>* cancel, SkipDir skipDir,
                      OnFile&& onFile) {
  std::error_code ec;
  auto st = fs::symlink_status(root, ec);
  if (ec) return;
  if (fs::is_symlink(st)) return;
  if (fs::is_regular_file(st)) {
    onFile(root);
    return;
  }
  if (!fs::is_directory(st)) return;

  std::vector<fs::path> dirs;
  dirs.push_back(root);
  const auto opts = fs::directory_options::skip_permission_denied;
  while (!dirs.empty()) {
    if (cancel && cancel->load()) return;
    fs::path dir = std::move(dirs.back());
    dirs.pop_back();
    std::error_code dec;
    fs::directory_iterator it(dir, opts, dec);
    if (dec) continue;
    for (; it != fs::directory_iterator(); it.increment(dec)) {
      if (dec) {
        dec.clear();
        break;
      }
      if (cancel && cancel->load()) return;
      const auto& entry = *it;
      std::error_code lec;
      if (entry.is_symlink(lec)) continue;
      if (entry.is_directory(lec)) {
        auto name = entry.path().filename().string();
        if (skipDir(name)) continue;
        dirs.push_back(entry.path());
        continue;
      }
      if (entry.is_regular_file(lec)) onFile(entry.path());
    }
  }
}

}  // namespace

bool skipDirectoryName(const std::string& name) {
  return name == ".git" || name == ".svn" || name == ".hg" || name == ".Trash" ||
         name == "node_modules" || name == "Pods" || name == ".build" || name == "Carthage" ||
         name == ".photoslibrary" || name == ".fseventsd" || name == ".Spotlight-V100" ||
         name == ".DocumentRevisions-V100" || name == ".TemporaryItems" || name == ".MobileBackups";
}

SizeCount directorySize(const std::string& path, std::atomic<bool>* cancel,
                        const ProgressFn& progress) {
  SizeCount out;
  walkRegularFiles(
      fs::path(path), cancel, skipGitSvn,
      [&](const fs::path& file) {
        std::error_code lec;
        auto sz = fs::file_size(file, lec);
        if (lec) return;
        out.bytes += sz;
        out.files += 1;
        if (progress && (out.files % 64 == 0)) progress(file.string(), out.files, out.bytes);
      });
  return out;
}

SizeCount directoryAllocatedSize(const std::string& path, std::atomic<bool>* cancel,
                                 const ProgressFn& progress) {
  SizeCount out;
  walkRegularFiles(
      fs::path(path), cancel, skipDirectoryName,
      [&](const fs::path& file) {
        std::error_code lec;
        auto sz = allocatedBytes(file, lec);
        if (lec) return;
        out.bytes += sz;
        out.files += 1;
        if (progress && (out.files % 64 == 0)) progress(file.string(), out.files, out.bytes);
      });
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
