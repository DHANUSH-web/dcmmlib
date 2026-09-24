#include "dcmm/engine.hpp"

#include "dcmm/catalog.hpp"
#include "dcmm/antigravity.hpp"
#include "dcmm/cursor.hpp"
#include "dcmm/vscode.hpp"

#include <chrono>

namespace dcmm {

Engine::Engine() = default;
Engine::~Engine() = default;

void Engine::cancel() { cancel_.store(true); }
void Engine::resetCancel() { cancel_.store(false); }
bool Engine::cancelled() const { return cancel_.load(); }

ScanReport Engine::scanCatalog(const std::vector<CatalogEntry>& entries, const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  ScanReport report;
  auto t0 = std::chrono::steady_clock::now();
  for (const auto& e : entries) {
    if (cancel_.load()) break;
    auto g = scanCatalogEntry(e, &cancel_, progress);
    if (!g.items.empty()) report.groups.push_back(std::move(g));
  }
  report.sortBySizeDescending();
  auto t1 = std::chrono::steady_clock::now();
  report.elapsedMs =
      static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
  lastScan_ = report;
  return report;
}

ScanReport Engine::scanSmart(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  auto report = scanCatalog(smartCatalog(), progress);
  auto installers = scanInstallerLeftovers(&cancel_, progress);
  if (!installers.items.empty()) report.groups.push_back(std::move(installers));
  report.sortBySizeDescending();
  for (auto& g : report.groups)
    for (auto& it : g.items) it.selected = true;
  lastScan_ = report;
  return report;
}

ScanReport Engine::scanJunk(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return scanCatalog(junkCatalog(), progress);
}

ScanReport Engine::scanPrivacy(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return scanCatalog(privacyCatalog(), progress);
}

std::vector<VsCodeInstall> Engine::listVsCode(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return vsCodeInstalls(homeDirectory(), &cancel_, progress);
}

std::vector<VsCodeExtension> Engine::listVsCodeExtensions(VsCodeEdition edition,
                                                         const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return vsCodeExtensions(edition, homeDirectory(), &cancel_, progress);
}

std::vector<CursorInstall> Engine::listCursor(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return cursorInstalls(homeDirectory(), &cancel_, progress);
}

std::vector<CursorExtension> Engine::listCursorExtensions(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return cursorExtensions(homeDirectory(), &cancel_, progress);
}

std::vector<AntigravityInstall> Engine::listAntigravity(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return antigravityInstalls(homeDirectory(), &cancel_, progress);
}

std::vector<AntigravityExtension> Engine::listAntigravityExtensions(const ProgressFn& progress) {
  std::lock_guard<std::recursive_mutex> lock(mu_);
  resetCancel();
  return antigravityExtensions(homeDirectory(), &cancel_, progress);
}

}  // namespace dcmm
