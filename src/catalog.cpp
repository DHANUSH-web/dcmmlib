#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "dcmm/safety.hpp"
#include "walk.hpp"

#include <cstdlib>
#include <sys/stat.h>

#if defined(__APPLE__)
#include <unistd.h>
#endif

namespace dcmm {
namespace {

void addIfExists(std::vector<CatalogEntry>& out, CatalogEntry e) {
  e.path = expandPath(e.path);
  if (pathExists(e.path)) out.push_back(std::move(e));
}

#if defined(__APPLE__)
std::string darwinDir(int name) {
  char buf[1024];
  std::size_t n = confstr(name, buf, sizeof(buf));
  if (n == 0 || n > sizeof(buf)) return {};
  std::string s(buf);
  while (!s.empty() && s.back() == '/') s.pop_back();
  return s;
}
#endif

#if defined(_WIN32)
std::string windowsLocalAppData() {
  // DCMM_HOME isolates tests from the real profile, including LocalAppData.
  if (const char* o = std::getenv("DCMM_HOME"); o && *o)
    return joinPath(o, "AppData/Local");
  if (const char* local = std::getenv("LOCALAPPDATA"); local && *local) return local;
  return {};
}
#endif

}  // namespace

std::vector<CatalogEntry> smartCatalog() {
  std::vector<CatalogEntry> out;
  const std::string home = homeDirectory();
#if defined(__APPLE__)
  addIfExists(out, {"user_caches", "User Caches",
                    "Everything in your Library/Caches except files you do not own",
                    joinPath(home, "Library/Caches"), false, false, false});
  addIfExists(out, {"logs", "User Logs", "Application logs in your home folder — regenerable",
                    joinPath(home, "Library/Logs"), false, false, false});
  addIfExists(out, {"saved_state", "Saved Application State",
                    "Window positions and crash restoration data",
                    joinPath(home, "Library/Saved Application State"), false, false, false});
#elif defined(_WIN32)
  {
    const std::string local = windowsLocalAppData();
    if (!local.empty())
      addIfExists(out, {"temp", "User Temp", "Temporary files in your profile",
                        joinPath(local, "Temp"), false, false, false});
  }
#else
  addIfExists(out, {"user_cache", "User Cache", "XDG cache directory", joinPath(home, ".cache"),
                    false, false, false});
#endif
  return out;
}

std::vector<CatalogEntry> junkCatalog() {
  std::vector<CatalogEntry> out;
  const std::string home = homeDirectory();

#if defined(__APPLE__)
  addIfExists(out, {"user_caches", "User Caches", "App caches in your Library — safe to rebuild",
                    joinPath(home, "Library/Caches"), true, false, false});
  {
    auto userCache = darwinDir(_CS_DARWIN_USER_CACHE_DIR);
    if (!userCache.empty())
      addIfExists(out, {"darwin_cache", "System User Cache",
                        "Per-user cache directory used by macOS services", userCache, true, false,
                        true});
    auto userTemp = darwinDir(_CS_DARWIN_USER_TEMP_DIR);
    if (!userTemp.empty())
      addIfExists(out, {"darwin_temp", "Temporary Files", "Your per-user temporary directory",
                        userTemp, true, false, false});
  }
  addIfExists(out, {"logs", "User Logs", "Application and system logs in your home folder",
                    joinPath(home, "Library/Logs"), true, false, false});
  addIfExists(out, {"saved_state", "Saved Application State",
                    "Window positions and crash restoration data",
                    joinPath(home, "Library/Saved Application State"), true, false, false});
  addIfExists(out, {"xcode_derived", "Xcode Derived Data",
                    "Build products and indexes Xcode recreates",
                    joinPath(home, "Library/Developer/Xcode/DerivedData"), true, false, false});
  addIfExists(out, {"ios_support", "iOS Device Support",
                    "Symbols for devices you have plugged in — large, regenerable",
                    joinPath(home, "Library/Developer/Xcode/iOS DeviceSupport"), true, true, false});
  addIfExists(out, {"xcode_archives", "Xcode Archives",
                    "App Store archives — review before removing",
                    joinPath(home, "Library/Developer/Xcode/Archives"), true, true, false});
  addIfExists(out, {"sim_caches", "Simulator Caches", "Core Simulator download and runtime caches",
                    joinPath(home, "Library/Developer/CoreSimulator/Caches"), true, false, false});
  addIfExists(out, {"brew_opt", "Homebrew Cellar Cache", "Homebrew prefix cache (not Library/Caches)",
                    "/opt/homebrew/var/cache", true, false, false});
  addIfExists(out, {"trash", "Trash", "Items already in the Trash", joinPath(home, ".Trash"), true,
                    false, false});
#elif defined(_WIN32)
  {
    const std::string local = windowsLocalAppData();
    if (!local.empty()) {
      addIfExists(out, {"temp", "User Temp", "Temporary files", joinPath(local, "Temp"), true, false,
                        false});
      addIfExists(out, {"pip", "pip Cache", "Python wheels", joinPath(local, "pip/Cache"), true,
                        false, false});
    }
  }
#else
  addIfExists(out, {"user_cache", "User Cache", "XDG cache directory", joinPath(home, ".cache"),
                    true, false, true});
  addIfExists(out, {"thumbnails", "Thumbnails", "Freedesktop thumbnail cache",
                    joinPath(home, ".cache/thumbnails"), true, false, false});
  addIfExists(out, {"trash", "Trash", "Freedesktop trash", joinPath(home, ".local/share/Trash/files"),
                    true, false, false});
#endif

  addIfExists(out, {"pip", "Python pip Cache", "Downloaded Python wheels",
                    joinPath(home, ".cache/pip"), true, false, false});
  addIfExists(out, {"npm", "npm Cache", "npm package cache (_cacache)",
                    joinPath(home, ".npm/_cacache"), false, false, false});
  addIfExists(out, {"npm_logs", "npm Logs", "npm debug logs", joinPath(home, ".npm/_logs"), true,
                    false, false});
  addIfExists(out, {"gradle", "Gradle Cache", "Gradle dependency caches",
                    joinPath(home, ".gradle/caches"), true, false, false});
  addIfExists(out, {"cargo", "Cargo Registry Cache", "Rust crate download cache",
                    joinPath(home, ".cargo/registry/cache"), true, false, false});
  addIfExists(out, {"composer", "Composer Cache", "PHP Composer cache",
                    joinPath(home, ".composer/cache"), true, false, false});
  return out;
}

std::vector<CatalogEntry> privacyCatalog() {
  std::vector<CatalogEntry> out;
  const std::string home = homeDirectory();
#if defined(__APPLE__)
  const std::string caches = joinPath(home, "Library/Caches");
  const std::string support = joinPath(home, "Library/Application Support");
  addIfExists(out, {"safari_cache", "Safari Cache", "Safari website caches",
                    joinPath(caches, "com.apple.Safari"), true, false, false});
  addIfExists(out, {"chrome_cache", "Google Chrome Cache", "Chrome HTTP cache",
                    joinPath(support, "Google/Chrome/Default/Cache"), false, false, false});
  addIfExists(out, {"chrome_code", "Google Chrome Code Cache", "V8 code cache",
                    joinPath(support, "Google/Chrome/Default/Code Cache"), false, false, false});
  addIfExists(out, {"edge_cache", "Microsoft Edge Cache", "Edge HTTP cache",
                    joinPath(support, "Microsoft Edge/Default/Cache"), false, false, false});
  addIfExists(out, {"brave_cache", "Brave Cache", "Brave HTTP cache",
                    joinPath(support, "BraveSoftware/Brave-Browser/Default/Cache"), false, false,
                    false});
  addIfExists(out, {"firefox_cache", "Firefox Cache", "Firefox cache2", joinPath(caches, "Firefox"),
                    true, false, false});
  addIfExists(out, {"cookies", "Cookie Files", "System cookie store (review first)",
                    joinPath(home, "Library/Cookies"), true, true, false});
#else
  addIfExists(out, {"chrome", "Chrome Cache", "Chromium-based browser cache",
                    joinPath(home, ".cache/google-chrome"), true, false, false});
  addIfExists(out, {"firefox", "Firefox Cache", "Firefox cache",
                    joinPath(home, ".cache/mozilla"), true, false, false});
#endif
  return out;
}

ScanGroup scanCatalogEntry(const CatalogEntry& entry, std::atomic<bool>* cancel,
                           const ProgressFn& progress) {
  ScanGroup g;
  g.id = entry.groupId;
  g.title = entry.groupTitle;
  g.subtitle = entry.groupSubtitle;
  g.reviewFirst = entry.reviewFirst;

  auto addItem = [&](const std::string& path, const std::string& name) {
    if (cancel && cancel->load()) return;
    if (pathExists(path) && !isOwnedByCurrentUser(path)) return;
    if (entry.skipBrowsers && isBrowserCacheName(name)) return;
    auto sc = directorySize(path, cancel, progress);
    if (sc.bytes == 0 && sc.files == 0) return;
    ScanItem it;
    it.path = path;
    it.displayName = name.empty() ? displayName(path) : name;
    it.detail = path;
    it.bytes = sc.bytes;
    it.fileCount = sc.files ? sc.files : 1;
    it.selected = false;
    it.reviewFirst = entry.reviewFirst;
    g.items.push_back(std::move(it));
  };

  if (!pathExists(entry.path)) return g;
  if (entry.childrenAsItems && isDirectory(entry.path)) {
    forEachChild(entry.path, [&](const std::string& name, const std::string& full, bool) {
      addItem(full, name);
    });
  } else if (isDirectory(entry.path)) {
    SizeCount total;
    forEachChild(entry.path, [&](const std::string& name, const std::string& full, bool) {
      if (cancel && cancel->load()) return;
      if (pathExists(full) && !isOwnedByCurrentUser(full)) return;
      if (entry.skipBrowsers && isBrowserCacheName(name)) return;
      auto sc = directorySize(full, cancel, progress);
      total.bytes += sc.bytes;
      total.files += sc.files;
    });
    if (total.bytes == 0 && total.files == 0) return g;
    ScanItem it;
    it.path = entry.path;
    it.displayName = entry.groupTitle;
    it.detail = entry.path;
    it.bytes = total.bytes;
    it.fileCount = total.files ? total.files : 1;
    it.selected = false;
    it.reviewFirst = entry.reviewFirst;
    g.items.push_back(std::move(it));
  } else {
    addItem(entry.path, entry.groupTitle);
  }
  return g;
}

}  // namespace dcmm
