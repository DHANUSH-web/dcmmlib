#pragma once

#include "dcmm/types.hpp"

#include <atomic>

namespace dcmm {

ScanGroup scanCatalogEntry(const CatalogEntry& entry, std::atomic<bool>* cancel,
                           const ProgressFn& progress);
ScanGroup scanInstallerLeftovers(std::atomic<bool>* cancel, const ProgressFn& progress);

}  // namespace dcmm
