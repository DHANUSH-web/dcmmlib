#pragma once

#include "dcmm/types.hpp"

#include <atomic>
#include <string>
#include <vector>

namespace dcmm {

class Engine {
 public:
  Engine();
  ~Engine();

  Engine(const Engine&) = delete;
  Engine& operator=(const Engine&) = delete;

  void cancel();
  void resetCancel();
  bool cancelled() const;

  ScanReport scanJunk(const ProgressFn& progress = nullptr);
  ScanReport scanPrivacy(const ProgressFn& progress = nullptr);

  ScanReport& lastScan() { return lastScan_; }
  const ScanReport& lastScan() const { return lastScan_; }

  CleanResult trashPaths(const std::vector<std::string>& paths);
  CleanResult trashSelected();

  std::vector<LargeFile> findLargeFiles(const LargeFileOptions& opt,
                                        const ProgressFn& progress = nullptr);
  std::vector<DuplicateGroup> findDuplicates(const DuplicateOptions& opt,
                                             const ProgressFn& progress = nullptr);
  std::vector<SpaceNode> spaceLens(const ProgressFn& progress = nullptr);

  std::vector<InstalledApp> listApps(const ProgressFn& progress = nullptr);
  void attachLeftovers(InstalledApp& app);

  DiskStats disk(const std::string& path = {}) const;
  MemoryStats memory() const;

  std::vector<MaintenanceTask> maintenanceTasks() const;
  MaintenanceResult previewMaintenance(const std::string& id) const;
  MaintenanceResult runMaintenance(const std::string& id);

 private:
  ScanReport scanCatalog(const std::vector<CatalogEntry>& entries, const ProgressFn& progress);

  std::atomic<bool> cancel_{false};
  ScanReport lastScan_;
};

std::vector<CatalogEntry> junkCatalog();
std::vector<CatalogEntry> privacyCatalog();

}  // namespace dcmm
