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

}  // namespace

std::vector<MaintenanceTask> Engine::maintenanceTasks() const {
  return {
      {"empty_trash", "Empty Trash",
       "Permanently erase items currently in Trash.", "This cannot be undone."},
      {"flush_dns", "Flush DNS Cache", "Reset the local DNS resolver cache.",
       "Platform-specific; may require extra privileges."},
#if defined(__APPLE__)
      {"launch_services", "Rebuild Launch Services",
       "Refresh the database that maps documents to apps.", "Runs lsregister for the user domain."},
      {"quicklook", "Clear Quick Look Cache", "Delete thumbnail and preview caches.", ""},
#endif
  };
}

std::string Engine::runMaintenance(const std::string& id) {
  if (id == "empty_trash") {
    std::error_code ec;
#if defined(__APPLE__)
    fs::path trash = joinPath(homeDirectory(), ".Trash");
#elif defined(_WIN32)
    return "Use the Recycle Bin UI on Windows, or trash items through dcmm_trash_paths.";
#else
    fs::path trash = joinPath(homeDirectory(), ".local/share/Trash/files");
#endif
    std::size_t ok = 0, fail = 0;
    for (auto& e : fs::directory_iterator(trash, fs::directory_options::skip_permission_denied, ec)) {
      fs::remove_all(e.path(), ec);
      if (ec)
        fail++;
      else
        ok++;
    }
    std::ostringstream ss;
    ss << "Removed " << ok << " item" << (ok == 1 ? "" : "s") << " from Trash";
    if (fail) ss << " (" << fail << " failed)";
    ss << ".";
    return ss.str();
  }
  if (id == "flush_dns") {
#if defined(__APPLE__)
    return runShell("dscacheutil -flushcache 2>&1");
#elif defined(_WIN32)
    return runShell("ipconfig /flushdns");
#else
    return runShell("resolvectl flush-caches 2>&1 || systemd-resolve --flush-caches 2>&1");
#endif
  }
#if defined(__APPLE__)
  if (id == "launch_services") {
    return runShell(
        "/System/Library/Frameworks/CoreServices.framework/Frameworks/LaunchServices.framework/"
        "Support/lsregister -kill -r -domain user 2>&1");
  }
  if (id == "quicklook") {
    std::vector<std::string> paths = {
        joinPath(homeDirectory(), "Library/Caches/com.apple.QuickLook.thumbnailcache"),
        joinPath(homeDirectory(), "Library/Caches/com.apple.QuickLookDaemon")};
    auto r = trashPaths(paths);
    std::ostringstream ss;
    ss << "Moved " << r.trashedItems << " cache folder(s) to Trash ("
       << formatBytes(r.trashedBytes) << ").";
    return ss.str();
  }
#endif
  return "Unknown or unsupported task on this platform.";
}

}  // namespace dcmm
