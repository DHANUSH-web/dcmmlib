#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "walk.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <functional>
#include <sstream>
#include <string>
#include <system_error>
#include <vector>

#if !defined(_WIN32)
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#endif

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

bool isolatedHome() {
  const char* h = std::getenv("DCMM_HOME");
  return h && *h;
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

std::string stripSlash(std::string s) {
  while (s.size() > 1 && (s.back() == '/' || s.back() == '\\')) s.pop_back();
  return s;
}

bool isTrashMetadataName(const std::string& name) {
  return name == ".DS_Store" || name == ".localized" || name == ".hidden" ||
         (name.size() >= 2 && name[0] == '.' && name[1] == '_');
}

bool trashEntrySafe(const fs::path& trash, const fs::path& entry) {
  std::error_code ec;
  auto t = fs::weakly_canonical(trash, ec);
  if (ec || t.empty()) t = trash;
  auto e = fs::weakly_canonical(entry, ec);
  if (ec || e.empty()) e = entry;
  const std::string ts = stripSlash(t.string());
  const std::string es = stripSlash(e.string());
  if (es == ts) return false;
  if (es.size() < ts.size()) return false;
  if (es.compare(0, ts.size(), ts) != 0) return false;
  return es.size() == ts.size() || es[ts.size()] == '/' || es[ts.size()] == '\\';
}

std::vector<fs::path> trashRoots() {
  std::vector<fs::path> roots;
  auto add = [&](fs::path p) {
    std::error_code ec;
    if (p.empty() || !fs::exists(p, ec)) return;
    auto c = fs::weakly_canonical(p, ec);
    if (ec || c.empty()) c = p;
    for (const auto& r : roots)
      if (r == c) return;
    roots.push_back(c);
  };
  add(userTrashDir());
#if defined(__APPLE__)
  if (!isolatedHome()) {
    std::error_code ec;
    const auto uid = std::to_string(getuid());
    fs::directory_iterator it("/Volumes", fs::directory_options::skip_permission_denied, ec);
    for (; it != fs::directory_iterator() && !ec; it.increment(ec)) {
      add(it->path() / ".Trashes" / uid);
    }
  }
#endif
  return roots;
}

enum class TrashListStatus { Ok, Denied, Missing };

TrashListStatus forEachTrashChild(const fs::path& trash,
                                  const std::function<void(const fs::path&)>& fn) {
  std::error_code ec;
  if (trash.empty() || !fs::exists(trash, ec)) return TrashListStatus::Missing;
#if defined(_WIN32)
  fs::directory_iterator it(trash, fs::directory_options::skip_permission_denied, ec);
  if (ec) return TrashListStatus::Denied;
  for (; it != fs::directory_iterator() && !ec; it.increment(ec)) fn(it->path());
  return TrashListStatus::Ok;
#else
  DIR* dir = opendir(trash.c_str());
  if (!dir) {
    if (errno == EACCES || errno == EPERM) return TrashListStatus::Denied;
    return TrashListStatus::Missing;
  }
  while (dirent* ent = readdir(dir)) {
    if (std::strcmp(ent->d_name, ".") == 0 || std::strcmp(ent->d_name, "..") == 0) continue;
    fn(trash / ent->d_name);
  }
  closedir(dir);
  return TrashListStatus::Ok;
#endif
}

#if defined(__APPLE__)
long finderTrashCount() {
  if (isolatedHome()) return -1;
  FILE* pipe =
      popen("/usr/bin/osascript -e 'tell application \"Finder\" to count items of trash'", "r");
  if (!pipe) return -1;
  char buf[128]{};
  const char* got = fgets(buf, sizeof(buf), pipe);
  pclose(pipe);
  if (!got) return -1;
  char* end = nullptr;
  long n = std::strtol(buf, &end, 10);
  if (end == buf || n < 0) return -1;
  return n;
}

bool finderEmptyTrash() {
  if (isolatedHome()) return false;
  std::string out = runShell(
      "osascript -e 'tell application \"Finder\"' -e 'try' -e 'empty the trash' -e "
      "'return \"ok\"' -e 'on error' -e 'return \"err\"' -e 'end try' -e 'end tell'");
  return out.find("ok") != std::string::npos;
}
#endif

struct TrashMeasure {
  uint64_t bytes = 0;
  uint64_t items = 0;
  bool denied = false;
  bool usedFinder = false;
};

TrashMeasure measureTrash() {
  TrashMeasure m;
  for (const auto& root : trashRoots()) {
    auto st = forEachTrashChild(root, [&](const fs::path& p) {
      if (!trashEntrySafe(root, p)) return;
      if (isTrashMetadataName(p.filename().string())) return;
      auto sc = directorySize(p.string());
      m.bytes += sc.bytes;
      m.items += 1;
    });
    if (st == TrashListStatus::Denied) m.denied = true;
  }
#if defined(__APPLE__)
  long n = finderTrashCount();
  if (n >= 0) {
    m.usedFinder = true;
    if (n == 0) {
      m.items = 0;
      m.bytes = 0;
    } else if (static_cast<uint64_t>(n) > m.items) {
      m.items = static_cast<uint64_t>(n);
    }
  }
#endif
  return m;
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
    auto m = measureTrash();
    r.bytesFreed = m.bytes;
    r.itemsAffected = m.items;
    if (m.items > 0 || m.bytes > 0) {
      r.nothingToDo = false;
      r.message = m.bytes > 0 ? (std::string("Trash currently holds ") + formatBytes(m.bytes) +
                                 " in " + formatCount(m.items, "item", "items") + ".")
                              : (std::string("Trash currently holds ") +
                                 formatCount(m.items, "item", "items") + ".");
      return r;
    }
    r.nothingToDo = true;
    r.message = "Nothing to clean. Trash is already empty.";
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
    uint64_t ok = 0, fail = 0, bytes = 0;
#if defined(__APPLE__)
    bool finderOk = finderEmptyTrash();
#else
    bool finderOk = false;
#endif
    for (const auto& root : trashRoots()) {
      auto st = forEachTrashChild(root, [&](const fs::path& p) {
        if (!trashEntrySafe(root, p)) {
          fail++;
          return;
        }
        auto sc = directorySize(p.string());
        std::error_code rec;
        fs::remove_all(p, rec);
        if (rec)
          fail++;
        else {
          ok++;
          bytes += sc.bytes;
        }
      });
      if (st == TrashListStatus::Denied) fail++;
    }
    out.bytesFreed = bytes;
    out.itemsAffected = ok;
    std::ostringstream ss;
    if (finderOk && ok == 0) {
      ss << "Finder emptied the Trash.";
    } else if (ok == 0 && fail == 0 && !finderOk) {
      out.nothingToDo = true;
      ss << "Nothing to clean. Trash is already empty.";
    } else {
      ss << "Freed " << formatBytes(bytes) << " from Trash ("
         << formatCount(ok, "item", "items") << ")";
      if (fail) ss << "; " << fail << " skipped";
      if (finderOk) ss << "; Finder emptied remaining items";
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
