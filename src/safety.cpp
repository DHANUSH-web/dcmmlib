#include "dcmm/safety.hpp"

#include "dcmm/path.hpp"

#include <cctype>
#include <string>
#include <vector>

#if !defined(_WIN32)
#include <sys/stat.h>
#include <unistd.h>
#endif

namespace dcmm {
namespace {

std::string norm(const std::string& p) {
  auto r = resolveRealPath(p);
  while (r.size() > 1 && (r.back() == '/' || r.back() == '\\')) r.pop_back();
  return r;
}

#if defined(_WIN32)
char pathSepNorm(char c) {
  return (c == '/' || c == '\\') ? '\\' : c;
}
#endif

bool hasPrefix(const std::string& path, const std::string& prefix) {
  if (prefix.empty() || path.size() < prefix.size()) return false;
#if defined(_WIN32)
  for (std::size_t i = 0; i < prefix.size(); ++i) {
    unsigned char a = static_cast<unsigned char>(pathSepNorm(path[i]));
    unsigned char b = static_cast<unsigned char>(pathSepNorm(prefix[i]));
    if (std::tolower(a) != std::tolower(b)) return false;
  }
#else
  if (path.compare(0, prefix.size(), prefix) != 0) return false;
#endif
  return path.size() == prefix.size() || path[prefix.size()] == '/' || path[prefix.size()] == '\\';
}

bool isVolumeRoot(const std::string& path) {
  if (path.empty() || path == "/" || path == "\\") return true;
#if defined(_WIN32)
  // weakly_canonical("/") is "C:\" then trailing slash stripped -> "C:"
  if (path.size() == 2 && std::isalpha(static_cast<unsigned char>(path[0])) && path[1] == ':')
    return true;
#endif
  return false;
}

std::string lowerCopy(std::string s) {
  for (char& c : s) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
  return s;
}

std::string lastComponent(const std::string& path) {
  auto pos = path.find_last_of("/\\");
  return pos == std::string::npos ? path : path.substr(pos + 1);
}

}  // namespace

bool isInstallerFileName(const std::string& name) {
  const std::string n = lowerCopy(name);
  static const char* ext[] = {".dmg", ".pkg", ".mpkg", ".msi", ".deb", ".rpm", nullptr};
  for (int i = 0; ext[i]; ++i) {
    auto e = ext[i];
    auto elen = std::char_traits<char>::length(e);
    if (n.size() >= elen && n.compare(n.size() - elen, elen, e) == 0) return true;
  }
  return false;
}

bool isSensitiveFileName(const std::string& name) {
  const std::string n = lowerCopy(name);
  static const char* exact[] = {"id_rsa",     "id_dsa", "id_ecdsa", "id_ed25519", ".env",
                                "authorized_keys", "known_hosts", "credentials", nullptr};
  for (int i = 0; exact[i]; ++i)
    if (n == exact[i]) return true;
  static const char* ext[] = {".pem",     ".key",  ".p12", ".pfx", ".p8",
                              ".kdbx",    ".cer",  ".crt", ".asc", ".gpg",
                              ".sparsebundle", ".sparseimage", ".wallet", nullptr};
  for (int i = 0; ext[i]; ++i) {
    auto e = ext[i];
    auto elen = std::char_traits<char>::length(e);
    if (n.size() >= elen && n.compare(n.size() - elen, elen, e) == 0) return true;
  }
  return false;
}

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
  const std::string home = norm(homeDirectory());
  if (path.empty() || isVolumeRoot(path) || path == home) return true;

#if defined(_WIN32)
  if (path == "C:\\Windows" || path == "C:\\Program Files" || path == "C:\\Program Files (x86)")
    return true;
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
                                       "/System/Applications",
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
                                    "Library/Safari",
                                    "Library/Messages",
                                    "Library/Calendars",
                                    "Library/Reminders",
                                    "Library/Accounts",
                                    "Library/Application Support/AddressBook",
                                    "Library/Application Support/MobileSync",
                                    "Library/Application Support/com.apple.TCC",
                                    "Library/Application Support/iCloud",
                                    "Library/Application Support/CloudDocs",
                                    "Library/Mobile Documents",
                                    "Library/CloudStorage",
                                    "Library/Photos",
                                    "Library/HomeKit",
                                    "Library/IdentityServices",
                                    "Library/PassKit",
                                    "Library/Sharing",
                                    "Library/IntelligencePlatform",
                                    "Library/Biome",
                                    "Library/Suggestions",
                                    "Library/Wallet",
                                    "Library/Group Containers/com.apple.bird",
                                    "Library/Containers/com.apple.mail",
                                    "Library/Containers/com.apple.MobileSMS",
                                    "Library/Containers/com.apple.Safari",
                                    ".ssh",
                                    ".gnupg",
                                    ".aws",
                                    ".config",
                                    ".kube",
                                    ".password-store",
                                    nullptr};
  for (int i = 0; sensitive[i]; ++i)
    if (hasPrefix(path, joinPath(home, sensitive[i]))) return true;

  if (path.find(".photoslibrary") != std::string::npos) return true;
  if (isSensitiveFileName(lastComponent(path))) return true;
  return false;
}

std::vector<std::string> junkCategoryRoots() {
  const std::string home = homeDirectory();
  return {
      joinPath(home, "Library/Caches"),
      joinPath(home, "Library/Logs"),
      joinPath(home, "Library/Saved Application State"),
      joinPath(home, "Library/HTTPStorages"),
      joinPath(home, "Library/WebKit"),
      joinPath(home, ".Trash"),
      joinPath(home, ".local/share/Trash"),
      joinPath(home, ".cache"),
      joinPath(home, ".npm/_cacache"),
      joinPath(home, ".npm/_logs"),
      joinPath(home, ".yarn/cache"),
      joinPath(home, ".composer/cache"),
      joinPath(home, ".gradle/caches"),
      joinPath(home, ".cargo/registry/cache"),
      joinPath(home, "Library/Developer/Xcode/DerivedData"),
      joinPath(home, "Library/Developer/Xcode/iOS DeviceSupport"),
      joinPath(home, "Library/Developer/Xcode/watchOS DeviceSupport"),
      joinPath(home, "Library/Developer/Xcode/tvOS DeviceSupport"),
      joinPath(home, "Library/Developer/Xcode/Archives"),
      joinPath(home, "Library/Developer/CoreSimulator/Caches"),
      "/opt/homebrew/var/cache",
      "/opt/homebrew/var/homebrew",
      "/usr/local/var/cache",
      "/private/var/tmp",
      "/tmp",
  };
}

bool isOwnedByCurrentUser(const std::string& raw) {
#if defined(_WIN32)
  (void)raw;
  return true;
#else
  struct stat st {};
  if (lstat(raw.c_str(), &st) != 0) return false;
  return st.st_uid == getuid();
#endif
}

bool isJunkCategoryRoot(const std::string& raw) {
  const std::string path = norm(raw);
  if (path.empty()) return false;
  for (const auto& p : junkCategoryRoots()) {
    if (path == norm(p)) return true;
  }
  return false;
}

bool isSafeToTrash(const std::string& raw) {
  if (isProtectedPath(raw)) return false;
  const std::string path = norm(raw);
  if (path.empty()) return false;
  if (isSensitiveFileName(lastComponent(path))) return false;
  if (pathExists(path) && !isOwnedByCurrentUser(path)) return false;

  const std::string home = norm(homeDirectory());

  // Regenerable junk only — never the category folder itself (children only).
  for (const auto& p : junkCategoryRoots()) {
    const std::string root = norm(p);
    if (hasPrefix(path, root) && path.size() != root.size()) return true;
  }

  if (path.find("/var/folders/") != std::string::npos &&
      (path.find("/C/") != std::string::npos || path.find("/T/") != std::string::npos))
    return true;

  // Uninstaller leftovers: one named child, not the parent folder, never Apple IDs.
  auto allowNamedChild = [&](const std::string& root, bool plistOnly) {
    if (!hasPrefix(path, root) || path.size() == root.size()) return false;
    std::string rest = path.substr(root.size() + 1);
    if (rest.find('/') != std::string::npos) return false;
    if (rest.rfind("com.apple.", 0) == 0) return false;
    static const char* generic[] = {"Google", "Apple",   "Microsoft", "Adobe",
                                    "Shared", "Common",  "Unity",     "Autodesk",
                                    nullptr};
    for (int i = 0; generic[i]; ++i)
      if (rest == generic[i]) return false;
    if (plistOnly) {
      auto low = lowerCopy(rest);
      return low.size() > 6 && low.compare(low.size() - 6, 6, ".plist") == 0;
    }
    return !rest.empty();
  };
  if (allowNamedChild(joinPath(home, "Library/Application Support"), false)) return true;
  if (allowNamedChild(joinPath(home, "Library/Preferences"), true)) return true;
  if (allowNamedChild(joinPath(home, "Library/LaunchAgents"), true)) return true;
  if (allowNamedChild(joinPath(home, "Library/Containers"), false)) return true;

  const auto downloads = joinPath(home, "Downloads");
  const auto documents = joinPath(home, "Documents");
  if (isInstallerFileName(lastComponent(path))) {
    if ((hasPrefix(path, downloads) && path.size() != downloads.size()) ||
        (hasPrefix(path, documents) && path.size() != documents.size()))
      return true;
  }

  // User-selected regular files (Large Files / Duplicates), never directories.
  if (isRegularFile(path) && hasPrefix(path, home)) return true;

  // Third-party .app bundles only — never /System/Applications.
  if (path.size() > 4 && path.compare(path.size() - 4, 4, ".app") == 0) {
    if (hasPrefix(path, "/System")) return false;
    if (hasPrefix(path, "/Applications") || hasPrefix(path, joinPath(home, "Applications")))
      return true;
  }
  return false;
}

}  // namespace dcmm
