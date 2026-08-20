#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "walk.hpp"

#include <array>
#include <cstdio>
#include <filesystem>
#include <sstream>
#include <system_error>

namespace dcmm {
namespace fs = std::filesystem;

namespace {

std::string runShell(const char* cmd) {
  std::array<char, 512> buf{};
  std::string out;
#if defined(_WIN32)
  FILE* pipe = _popen(cmd, "r");
#else
  FILE* pipe = popen(cmd, "r");
#endif
  if (!pipe) return "Failed to start command.";
  while (fgets(buf.data(), static_cast<int>(buf.size()), pipe)) out += buf.data();
#if defined(_WIN32)
  int st = _pclose(pipe);
#else
  int st = pclose(pipe);
#endif
  if (out.empty()) out = (st == 0) ? "Done." : "Finished with errors.";
  return out;
}

fs::path userTrashDir() {
#if defined(__APPLE__)
  return joinPath(homeDirectory(), ".Trash");
#elif defined(_WIN32)
  return {};
#else
  return joinPath(homeDirectory(), ".local/share/Trash/files");
#endif
}

bool trashEntrySafe(const fs::path& trash, const fs::path& entry) {
  std::error_code rec;
  auto resolved = fs::weakly_canonical(entry, rec);
  if (rec) return false;
  std::string rs = resolved.string();
  std::string ts = trash.string();
  return rs.rfind(ts, 0) == 0;
}

void measureTrash(uint64_t& bytes, uint64_t& items) {
  bytes = 0;
  items = 0;
  auto trash = userTrashDir();
  std::error_code ec;
  if (trash.empty() || !fs::exists(trash, ec)) return;
  for (auto& e : fs::directory_iterator(trash, fs::directory_options::skip_permission_denied, ec)) {
    if (!trashEntrySafe(trash, e.path())) continue;
    auto sc = directorySize(e.path().string());
    bytes += sc.bytes;
    items += 1;
  }
}

std::vector<std::string> quickLookCachePaths() {
  return {
      joinPath(homeDirectory(), "Library/Caches/com.apple.QuickLook.thumbnailcache"),
      joinPath(homeDirectory(), "Library/Caches/com.apple.QuickLookDaemon"),
  };
}

void measurePaths(const std::vector<std::string>& paths, uint64_t& bytes, uint64_t& items) {
  bytes = 0;
  items = 0;
  for (const auto& p : paths) {
    if (!pathExists(p)) continue;
    auto sc = directorySize(p);
    if (sc.bytes == 0 && sc.files == 0) continue;
    bytes += sc.bytes;
    items += 1;
  }
}

}  // namespace

std::vector<MaintenanceTask> Engine::maintenanceTasks() const {
  return {
      {"empty_trash", "Empty Trash",
       "Permanently erase items currently in Trash.", "This cannot be undone."},
      {"flush_dns", "Flush DNS Cache", "Reset the local DNS resolver cache.",
       "Does not delete files."},
#if defined(__APPLE__)
      {"launch_services", "Rebuild Launch Services",
       "Refresh the database that maps documents to apps.", "Does not delete files."},
      {"quicklook", "Clear Quick Look Cache", "Move thumbnail and preview caches to Trash.", ""},
#endif
  };
}

MaintenanceResult Engine::previewMaintenance(const std::string& id) const {
  MaintenanceResult r;
  if (id == "empty_trash") {
    measureTrash(r.bytesFreed, r.itemsAffected);
    r.nothingToDo = r.itemsAffected == 0 && r.bytesFreed == 0;
    r.message = r.nothingToDo ? "Nothing to clean. Trash is already empty."
                              : (std::string("Trash currently holds ") +
                                 formatBytes(r.bytesFreed) + " in " +
                                 formatCount(r.itemsAffected, "item", "items") + ".");
    return r;
  }
  if (id == "quicklook") {
    measurePaths(quickLookCachePaths(), r.bytesFreed, r.itemsAffected);
    r.nothingToDo = r.bytesFreed == 0;
    r.message = r.nothingToDo ? "Nothing to clean. Quick Look caches are already empty."
                              : (std::string("Quick Look caches currently use ") +
                                 formatBytes(r.bytesFreed) + ".");
    return r;
  }
  if (id == "flush_dns") {
    r.nothingToDo = false;
    r.message = "Flushes local DNS lookups only. No files are deleted.";
    return r;
  }
  if (id == "launch_services") {
    r.nothingToDo = false;
    r.message = "Rebuilds the Open With database. No files are deleted.";
    return r;
  }
  r.nothingToDo = true;
  r.message = "Nothing to clean.";
  return r;
}

MaintenanceResult Engine::runMaintenance(const std::string& id) {
  auto preview = previewMaintenance(id);
  if (preview.nothingToDo) return preview;

  MaintenanceResult out;
  if (id == "empty_trash") {
#if defined(_WIN32)
    out.message = "Use the Recycle Bin UI on Windows.";
    out.nothingToDo = true;
    return out;
#else
    auto trash = userTrashDir();
    std::error_code ec;
    uint64_t ok = 0, fail = 0, bytes = 0;
    for (auto& e : fs::directory_iterator(trash, fs::directory_options::skip_permission_denied, ec)) {
      if (!trashEntrySafe(trash, e.path())) {
        fail++;
        continue;
      }
      auto sc = directorySize(e.path().string());
      fs::remove_all(e.path(), ec);
      if (ec)
        fail++;
      else {
        ok++;
        bytes += sc.bytes;
      }
    }
    out.bytesFreed = bytes;
    out.itemsAffected = ok;
    std::ostringstream ss;
    if (ok == 0 && fail == 0) {
      out.nothingToDo = true;
      ss << "Nothing to clean. Trash is already empty.";
    } else {
      ss << "Freed " << formatBytes(bytes) << " from Trash ("
         << formatCount(ok, "item", "items") << ")";
      if (fail) ss << "; " << fail << " skipped";
      ss << ".";
    }
    out.message = ss.str();
    return out;
#endif
  }
  if (id == "flush_dns") {
#if defined(__APPLE__)
    runShell("dscacheutil -flushcache 2>&1");
#elif defined(_WIN32)
    runShell("ipconfig /flushdns");
#else
    runShell("resolvectl flush-caches 2>&1 || systemd-resolve --flush-caches 2>&1");
#endif
    out.message = "DNS cache flushed. No files were deleted.";
    return out;
  }
#if defined(__APPLE__)
  if (id == "launch_services") {
    runShell(
        "/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/"
        "Support/lsregister -kill -r -domain user 2>&1");
    out.message = "Launch Services rebuilt. No files were deleted.";
    return out;
  }
  if (id == "quicklook") {
    auto r = trashPaths(quickLookCachePaths());
    out.bytesFreed = r.trashedBytes;
    out.itemsAffected = r.trashedItems;
    if (r.trashedItems == 0 && r.trashedBytes == 0) {
      out.nothingToDo = true;
      out.message = "Nothing to clean. Quick Look caches are already empty.";
    } else {
      std::ostringstream ss;
      ss << "Freed " << formatBytes(r.trashedBytes) << " by moving Quick Look caches to Trash.";
      out.message = ss.str();
    }
    return out;
  }
#endif
  out.nothingToDo = true;
  out.message = "Nothing to clean.";
  return out;
}

}  // namespace dcmm
