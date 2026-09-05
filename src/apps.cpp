#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "dcmm/safety.hpp"
#include "dcmm/walk.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>

#if defined(__APPLE__)
#include <CoreFoundation/CoreFoundation.h>
#endif

namespace dcmm {
namespace {

std::string readFile(const std::string& path, std::size_t max = 1 << 20) {
  std::ifstream in(path, std::ios::binary);
  if (!in) return {};
  std::string s;
  s.resize(max);
  in.read(s.data(), static_cast<std::streamsize>(max));
  s.resize(static_cast<std::size_t>(in.gcount()));
  return s;
}

std::string xmlPlistString(const std::string& xml, const std::string& key) {
  const std::string k = "<key>" + key + "</key>";
  auto pos = xml.find(k);
  if (pos == std::string::npos) return {};
  auto s = xml.find("<string>", pos + k.size());
  if (s == std::string::npos || s > pos + 200) return {};
  s += 8;
  auto e = xml.find("</string>", s);
  if (e == std::string::npos) return {};
  return xml.substr(s, e - s);
}

#if defined(__APPLE__)
std::string cfString(CFStringRef s) {
  if (!s) return {};
  char buf[512];
  if (CFStringGetCString(s, buf, sizeof(buf), kCFStringEncodingUTF8)) return buf;
  return {};
}

void readBinaryPlist(const std::string& path, InstalledApp& app) {
  CFURLRef url = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
                                                         reinterpret_cast<const UInt8*>(path.c_str()),
                                                         static_cast<CFIndex>(path.size()), false);
  if (!url) return;
  CFReadStreamRef stream = CFReadStreamCreateWithFile(kCFAllocatorDefault, url);
  CFRelease(url);
  if (!stream) return;
  CFReadStreamOpen(stream);
  CFPropertyListRef plist = CFPropertyListCreateWithStream(
      kCFAllocatorDefault, stream, 0, kCFPropertyListImmutable, nullptr, nullptr);
  CFReadStreamClose(stream);
  CFRelease(stream);
  if (!plist || CFGetTypeID(plist) != CFDictionaryGetTypeID()) {
    if (plist) CFRelease(plist);
    return;
  }
  auto dict = static_cast<CFDictionaryRef>(plist);
  auto get = [&](CFStringRef key) -> std::string {
    auto v = static_cast<CFStringRef>(CFDictionaryGetValue(dict, key));
    if (!v || CFGetTypeID(v) != CFStringGetTypeID()) return {};
    return cfString(v);
  };
  auto name = get(CFSTR("CFBundleDisplayName"));
  if (name.empty()) name = get(CFSTR("CFBundleName"));
  if (!name.empty()) app.name = name;
  auto bid = get(CFSTR("CFBundleIdentifier"));
  if (!bid.empty()) app.bundleId = bid;
  auto ver = get(CFSTR("CFBundleShortVersionString"));
  if (!ver.empty()) app.version = ver;
  CFRelease(plist);
}
#endif

[[maybe_unused]] void parseInfoPlist(const std::string& plistPath, InstalledApp& app) {
  auto body = readFile(plistPath);
  if (body.size() >= 8 && body.compare(0, 8, "bplist00") == 0) {
#if defined(__APPLE__)
    readBinaryPlist(plistPath, app);
#endif
    return;
  }
  auto name = xmlPlistString(body, "CFBundleDisplayName");
  if (name.empty()) name = xmlPlistString(body, "CFBundleName");
  if (!name.empty()) app.name = name;
  auto bid = xmlPlistString(body, "CFBundleIdentifier");
  if (!bid.empty()) app.bundleId = bid;
  auto ver = xmlPlistString(body, "CFBundleShortVersionString");
  if (!ver.empty()) app.version = ver;
}

ScanItem leftoverItem(const std::string& path) {
  ScanItem it;
  it.path = path;
  it.displayName = displayName(path);
  it.detail = path;
  auto sc = directorySize(path, nullptr, nullptr);
  it.bytes = sc.bytes;
  it.fileCount = sc.files ? sc.files : 1;
  it.selected = false;
  return it;
}

}  // namespace

std::vector<InstalledApp> Engine::listApps(const ProgressFn& progress) {
  std::vector<InstalledApp> apps;
  std::vector<std::string> roots;
#if defined(__APPLE__)
  roots = {"/Applications", joinPath(homeDirectory(), "Applications")};
#elif defined(_WIN32)
  if (const char* pf = std::getenv("ProgramFiles")) roots.push_back(pf);
  if (const char* pf86 = std::getenv("ProgramFiles(x86)")) roots.push_back(pf86);
#else
  roots = {"/usr/share/applications", joinPath(homeDirectory(), ".local/share/applications")};
#endif

  for (const auto& root : roots) {
    if (cancel_.load()) break;
    if (!isDirectory(root)) continue;
    forEachChild(root, [&](const std::string& name, const std::string& full, bool isDir) {
      if (cancel_.load()) return;
#if defined(__APPLE__)
      if (!isDir) return;
      if (name.size() < 4 || name.compare(name.size() - 4, 4, ".app") != 0) return;
      InstalledApp a;
      a.appPath = full;
      a.name = name.substr(0, name.size() - 4);
      parseInfoPlist(joinPath(joinPath(full, "Contents"), "Info.plist"), a);
      if (a.bundleId.rfind("com.apple.", 0) == 0) return;
      a.appBytes = directorySize(full, &cancel_, progress).bytes;
      apps.push_back(std::move(a));
#else
      (void)isDir;
      if (name.size() < 8 || name.compare(name.size() - 8, 8, ".desktop") != 0) {
        if (isDir) {
          InstalledApp a;
          a.appPath = full;
          a.name = name;
          a.appBytes = directorySize(full, &cancel_, progress).bytes;
          apps.push_back(std::move(a));
        }
        return;
      }
      InstalledApp a;
      a.appPath = full;
      a.name = name;
      auto body = readFile(full, 64 * 1024);
      auto np = body.find("\nName=");
      if (np != std::string::npos) {
        np += 6;
        auto e = body.find('\n', np);
        a.name = body.substr(np, e - np);
      }
      apps.push_back(std::move(a));
#endif
      if (progress) progress(full, apps.size(), 0);
    });
  }
  std::sort(apps.begin(), apps.end(), [](const InstalledApp& x, const InstalledApp& y) {
    if (x.appBytes != y.appBytes) return x.appBytes > y.appBytes;
    return x.name < y.name;
  });
  return apps;
}

void Engine::attachLeftovers(InstalledApp& app) {
  app.leftovers.clear();
  const std::string home = homeDirectory();
  auto consider = [&](const std::string& path) {
    if (path.empty() || !pathExists(path)) return;
    if (isProtectedPath(path)) return;
    app.leftovers.push_back(leftoverItem(path));
  };
  auto support = [&](const std::string& leaf) {
    consider(joinPath(joinPath(home, "Library/Application Support"), leaf));
    consider(joinPath(joinPath(home, "Library/Caches"), leaf));
    consider(joinPath(joinPath(home, "Library/Logs"), leaf));
    consider(joinPath(joinPath(home, "Library/Preferences"), leaf + ".plist"));
    consider(joinPath(joinPath(home, "Library/Saved Application State"), leaf + ".savedState"));
    consider(joinPath(joinPath(home, "Library/Containers"), leaf));
    consider(joinPath(joinPath(home, "Library/LaunchAgents"), leaf + ".plist"));
    consider(joinPath(joinPath(home, ".cache"), leaf));
  };
  if (!app.bundleId.empty()) support(app.bundleId);
  if (!app.name.empty() && app.name != app.bundleId) support(app.name);
  consider(app.appPath);
  std::sort(app.leftovers.begin(), app.leftovers.end(), [](const ScanItem& a, const ScanItem& b) {
    if (a.bytes != b.bytes) return a.bytes > b.bytes;
    return a.displayName < b.displayName;
  });
}

}  // namespace dcmm
