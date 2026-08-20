#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace dcmm {

struct ScanItem {
  std::string path;
  std::string displayName;
  std::string detail;
  uint64_t bytes = 0;
  uint64_t fileCount = 0;
  bool selected = true;
  bool reviewFirst = false;
};

struct ScanGroup {
  std::string id;
  std::string title;
  std::string subtitle;
  std::vector<ScanItem> items;
  bool reviewFirst = false;

  uint64_t totalBytes() const;
  uint64_t selectedBytes() const;
  uint64_t selectedFiles() const;
  std::size_t selectedCount() const;
};

struct ScanReport {
  std::vector<ScanGroup> groups;
  uint64_t elapsedMs = 0;
  std::string warning;

  uint64_t totalBytes() const;
  uint64_t selectedBytes() const;
  std::vector<std::string> selectedPaths() const;
};

using ProgressFn = std::function<void(const std::string& path, uint64_t visited, uint64_t bytesAcc)>;

struct CleanResult {
  uint64_t trashedBytes = 0;
  uint64_t trashedItems = 0;
  uint64_t failedItems = 0;
  std::vector<std::string> errors;
};

struct DuplicateFile {
  std::string path;
  uint64_t bytes = 0;
  bool keep = false;
};

struct DuplicateGroup {
  uint64_t bytes = 0;
  std::string hashHex;
  std::vector<DuplicateFile> files;
};

struct DuplicateOptions {
  std::vector<std::string> roots;
  uint64_t minBytes = 256ull * 1024ull;
  uint64_t maxFiles = 200000;
};

struct LargeFile {
  std::string path;
  uint64_t bytes = 0;
  int64_t mtime = 0;
  bool selected = false;
};

struct LargeFileOptions {
  std::vector<std::string> roots;
  uint64_t minBytes = 50ull * 1024ull * 1024ull;
  std::size_t limit = 300;
};

struct InstalledApp {
  std::string appPath;
  std::string name;
  std::string bundleId;
  std::string version;
  uint64_t appBytes = 0;
  std::vector<ScanItem> leftovers;
};

struct SpaceNode {
  std::string path;
  std::string name;
  uint64_t bytes = 0;
  bool isDir = true;
};

struct DiskStats {
  std::string volumeName;
  std::string mountPoint;
  uint64_t totalBytes = 0;
  uint64_t freeBytes = 0;
  uint64_t availableBytes = 0;
  uint64_t purgeableBytes = 0;
};

struct MemoryStats {
  uint64_t totalBytes = 0;
  uint64_t usedBytes = 0;
  uint64_t freeBytes = 0;
  uint64_t wiredBytes = 0;
  uint64_t compressedBytes = 0;
};

struct MaintenanceTask {
  std::string id;
  std::string title;
  std::string detail;
  std::string note;
};

struct CatalogEntry {
  std::string groupId;
  std::string groupTitle;
  std::string groupSubtitle;
  std::string path;
  bool childrenAsItems = true;
  bool reviewFirst = false;
  bool skipBrowsers = false;
};

}  // namespace dcmm
