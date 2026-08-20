#include "dcmm/safety.hpp"

#include "dcmm/path.hpp"

#include <algorithm>
#include <cctype>
#include <vector>

namespace dcmm {
namespace {

std::string norm(const std::string& p) {
  auto r = resolveRealPath(p);
  while (r.size() > 1 && (r.back() == '/' || r.back() == '\\')) r.pop_back();
  return r;
}

bool hasPrefix(const std::string& path, const std::string& prefix) {
  if (prefix.empty() || path.size() < prefix.size()) return false;
  if (path.compare(0, prefix.size(), prefix) != 0) return false;
  return path.size() == prefix.size() || path[prefix.size()] == '/' || path[prefix.size()] == '\\';
}

}  // namespace

bool isBrowserCacheName(const std::string& name) {
  static const char* ids[] = {"com.apple.Safari",
                              "com.apple.SafariTechnologyPreview",
                              "com.google.Chrome",
                              "com.google.Chrome.canary",
                              "org.mozilla.firefox",
                              "com.microsoft.edgemac",
                              "com.brave.Browser",
                              "company.thebrowser.Browser",
                              "com.operasoftware.Opera",
                              "com.kagi.orion",
                              "org.chromium.Chromium",
                              "google-chrome",
                              "chromium",
                              "firefox",
                              "microsoft-edge",
                              "BraveSoftware",
                              nullptr};
  for (int i = 0; ids[i]; ++i) {
    if (name == ids[i]) return true;
    if (name.rfind(std::string(ids[i]) + ".", 0) == 0) return true;
  }
  return name == "Google" || name == "Chromium" || name == "Firefox" || name == "Arc" ||
         name == "Microsoft Edge" || name == "Safari" || name == "BraveSoftware";
}

bool isProtectedPath(const std::string& raw) {
  const std::string path = norm(raw);
  const std::string home = homeDirectory();
  if (path.empty() || path == "/" || path == home) return true;

#if defined(_WIN32)
  if (hasPrefix(path, "C:\\Windows") || hasPrefix(path, "C:\\Program Files") ||
      hasPrefix(path, "C:\\Program Files (x86)")) {
    if (path == "C:\\Windows" || path == "C:\\Program Files" || path == "C:\\Program Files (x86)")
      return true;
  }
#else
  if (path == "/Applications" || path == "/System" || path == "/Library") return true;
  static const char* prefixBlocks[] = {"/System",
                                       "/bin",
                                       "/sbin",
                                       "/usr/bin",
                                       "/usr/sbin",
                                       "/usr/lib",
                                       "/usr/libexec",
                                       "/usr/share",
                                       "/etc",
                                       "/dev",
                                       "/boot",
                                       "/proc",
                                       "/sys",
                                       "/private/etc",
                                       "/private/var/db",
                                       "/private/var/vm",
                                       "/Library/Apple",
                                       "/Library/Documentation",
                                       "/System/Volumes",
                                       nullptr};
  for (int i = 0; prefixBlocks[i]; ++i)
    if (hasPrefix(path, prefixBlocks[i])) return true;
#endif

  if (path == joinPath(home, "Library") || path == joinPath(home, ".ssh") ||
      path == joinPath(home, ".gnupg"))
    return true;

  static const char* exactHome[] = {"Documents", "Desktop",  "Downloads", "Pictures",
                                    "Movies",    "Music",    "Public",    "Applications",
                                    "Library",   nullptr};
  for (int i = 0; exactHome[i]; ++i)
    if (path == joinPath(home, exactHome[i])) return true;

  static const char* sensitive[] = {"Library/Keychains",
                                    "Library/Mail",
                                    "Library/Messages",
                                    "Library/Calendars",
                                    "Library/Reminders",
                                    "Library/Accounts",
                                    "Library/Application Support/AddressBook",
                                    "Library/Application Support/MobileSync",
                                    "Library/Application Support/com.apple.TCC",
                                    "Library/Application Support/iCloud",
                                    "Library/Mobile Documents",
                                    "Library/CloudStorage",
                                    "Library/Photos",
                                    "Library/HomeKit",
                                    "Library/IdentityServices",
                                    "Library/PassKit",
                                    "Library/Sharing",
                                    "Library/IntelligencePlatform",
                                    "Library/Group Containers/com.apple.bird",
                                    "Library/Containers/com.apple.mail",
                                    "Library/Containers/com.apple.MobileSMS",
                                    ".ssh",
                                    ".gnupg",
                                    ".aws",
                                    ".config/gcloud",
                                    ".kube",
                                    ".password-store",
                                    nullptr};
  for (int i = 0; sensitive[i]; ++i)
    if (hasPrefix(path, joinPath(home, sensitive[i]))) return true;

  if (path.find(".photoslibrary") != std::string::npos) return true;
  return false;
}

bool isSafeToTrash(const std::string& raw) {
  if (isProtectedPath(raw)) return false;
  const std::string path = norm(raw);
  if (path.empty()) return false;
  const std::string home = homeDirectory();

  const std::vector<std::string> allow = {
      joinPath(home, "Library/Caches"),
      joinPath(home, "Library/Logs"),
      joinPath(home, "Library/Saved Application State"),
      joinPath(home, "Library/HTTPStorages"),
      joinPath(home, "Library/WebKit"),
      joinPath(home, "Library/Cookies"),
      joinPath(home, "Library/Developer/Xcode/DerivedData"),
      joinPath(home, "Library/Developer/Xcode/iOS DeviceSupport"),
      joinPath(home, "Library/Developer/Xcode/watchOS DeviceSupport"),
      joinPath(home, "Library/Developer/Xcode/tvOS DeviceSupport"),
      joinPath(home, "Library/Developer/Xcode/Archives"),
      joinPath(home, "Library/Developer/CoreSimulator/Caches"),
      joinPath(home, ".Trash"),
      joinPath(home, ".local/share/Trash"),
      joinPath(home, ".cache"),
      joinPath(home, ".npm/_cacache"),
      joinPath(home, ".npm/_logs"),
      joinPath(home, ".yarn/cache"),
      joinPath(home, ".composer/cache"),
      joinPath(home, ".gradle/caches"),
      joinPath(home, ".cargo/registry/cache"),
      joinPath(home, "Library/Application Support"),
      joinPath(home, "Library/Preferences"),
      joinPath(home, "Library/Containers"),
      joinPath(home, "Library/Group Containers"),
      joinPath(home, "Library/LaunchAgents"),
      joinPath(home, "Applications"),
      "/opt/homebrew/var/cache",
      "/opt/homebrew/var/homebrew",
      "/usr/local/var/cache",
      "/Library/Caches",
      "/private/var/tmp",
      "/tmp",
      "/Applications",
  };

  for (const auto& p : allow) {
    if (hasPrefix(path, p) && path != p) return true;
  }

  if (path.find("/var/folders/") != std::string::npos &&
      (path.find("/C/") != std::string::npos || path.find("/T/") != std::string::npos))
    return true;

  if (isRegularFile(path) && hasPrefix(path, home)) return true;

  if (path.size() > 4 && path.compare(path.size() - 4, 4, ".app") == 0) {
    if (hasPrefix(path, "/Applications") || hasPrefix(path, joinPath(home, "Applications")))
      return true;
  }
  return false;
}

}  // namespace dcmm
