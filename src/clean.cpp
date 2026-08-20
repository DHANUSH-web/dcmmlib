#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "dcmm/safety.hpp"
#include "walk.hpp"

#include <chrono>
#include <filesystem>
#include <sstream>
#include <system_error>

#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif

namespace dcmm {
namespace fs = std::filesystem;

namespace {

std::string trashDir() {
  if (const char* t = std::getenv("DCMM_TRASH"); t && *t) return t;
#if defined(_WIN32)
  return {};  // Recycle Bin via SHFileOperation
#elif defined(__APPLE__)
  return joinPath(homeDirectory(), ".Trash");
#else
  return joinPath(homeDirectory(), ".local/share/Trash/files");
#endif
}

bool moveToTrashPosix(const std::string& path, std::string& err) {
  std::error_code ec;
  fs::path src(path);
  if (!fs::exists(src, ec)) {
    err = "not found";
    return false;
  }
  const auto destRoot = fs::path(trashDir());
  fs::create_directories(destRoot, ec);
  std::string stem = src.filename().string();
  fs::path dest = destRoot / stem;
  int n = 1;
  while (fs::exists(dest, ec)) {
    dest = destRoot / (stem + " " + std::to_string(n++));
  }
  fs::rename(src, dest, ec);
  if (!ec) return true;
  err = "could not move to Trash (" + ec.message() + ")";
  return false;
}

#if defined(_WIN32)
bool moveToTrashWin(const std::string& path, std::string& err) {
  std::string p = path;
  p.push_back('\0');
  p.push_back('\0');
  SHFILEOPSTRUCTA op{};
  op.wFunc = FO_DELETE;
  op.pFrom = p.c_str();
  op.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_SILENT | FOF_NOERRORUI;
  int rc = SHFileOperationA(&op);
  if (rc != 0) {
    err = "SHFileOperation failed";
    return false;
  }
  return true;
}
#endif

}  // namespace

std::vector<std::string> expandTrashTargets(const std::string& p) {
  std::vector<std::string> out;
  if (p.empty()) return out;
  if (isJunkCategoryRoot(p)) {
    forEachChild(p, [&](const std::string&, const std::string& full, bool) {
      if (isSafeToTrash(full)) out.push_back(full);
    });
    return out;
  }
  if (isSafeToTrash(p)) out.push_back(p);
  return out;
}

CleanResult Engine::trashPaths(const std::vector<std::string>& paths) {
  CleanResult result;
  for (const auto& p : paths) {
    auto targets = expandTrashTargets(p);
    if (targets.empty()) {
      result.failedItems++;
      result.errors.push_back("Blocked (protected path): " + p);
      continue;
    }
    for (const auto& t : targets) {
      auto sc = directorySize(t, nullptr, nullptr);
      std::string err;
      bool ok = false;
#if defined(_WIN32)
      ok = moveToTrashWin(t, err);
#else
      ok = moveToTrashPosix(t, err);
#endif
      if (!ok) {
        result.failedItems++;
        result.errors.push_back(t + ": " + err);
        continue;
      }
      result.trashedItems++;
      result.trashedBytes += sc.bytes;
    }
  }
  return result;
}

CleanResult Engine::trashSelected() { return trashPaths(lastScan_.selectedPaths()); }

}  // namespace dcmm
