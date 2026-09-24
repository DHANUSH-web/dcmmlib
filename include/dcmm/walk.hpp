#pragma once

#include "dcmm/types.hpp"

#include <atomic>
#include <cstdint>
#include <functional>
#include <string>

namespace dcmm {

struct SizeCount {
  uint64_t bytes = 0;
  uint64_t files = 0;
};

bool skipDirectoryName(const std::string& name);

SizeCount directorySize(const std::string& path, std::atomic<bool>* cancel = nullptr,
                        const ProgressFn& progress = nullptr);

/// Bytes actually allocated on disk (st_blocks), not logical file_size.
/// iCloud dataless files and sparse holes therefore stay small.
SizeCount directoryAllocatedSize(const std::string& path, std::atomic<bool>* cancel = nullptr,
                                 const ProgressFn& progress = nullptr);

/// Allocated size of the whole tree. Does not skip node_modules, .git, etc.
SizeCount directoryAllocatedSizeAll(const std::string& path, std::atomic<bool>* cancel = nullptr,
                                    const ProgressFn& progress = nullptr);

/// One walk of `dir`: total allocated bytes and per-immediate-child totals. No skip list.
struct SpaceMeasure {
  uint64_t bytes = 0;
  std::vector<SpaceNode> children;
};
SpaceMeasure spaceLensMeasure(const std::string& dir, std::atomic<bool>* cancel = nullptr,
                              const ProgressFn& progress = nullptr);

void forEachChild(const std::string& dir,
                  const std::function<void(const std::string& name, const std::string& full,
                                           bool isDir)>& fn);

}  // namespace dcmm
