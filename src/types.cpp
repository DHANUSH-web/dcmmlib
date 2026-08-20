#include "dcmm/types.hpp"

#include <algorithm>

namespace dcmm {

uint64_t ScanGroup::totalBytes() const {
  uint64_t n = 0;
  for (const auto& i : items) n += i.bytes;
  return n;
}

uint64_t ScanGroup::selectedBytes() const {
  uint64_t n = 0;
  for (const auto& i : items)
    if (i.selected) n += i.bytes;
  return n;
}

uint64_t ScanGroup::selectedFiles() const {
  uint64_t n = 0;
  for (const auto& i : items)
    if (i.selected) n += i.fileCount ? i.fileCount : 1;
  return n;
}

std::size_t ScanGroup::selectedCount() const {
  std::size_t n = 0;
  for (const auto& i : items)
    if (i.selected) ++n;
  return n;
}

void ScanGroup::sortBySizeDescending() {
  std::sort(items.begin(), items.end(), [](const ScanItem& a, const ScanItem& b) {
    if (a.bytes != b.bytes) return a.bytes > b.bytes;
    return a.displayName < b.displayName;
  });
}

uint64_t ScanReport::totalBytes() const {
  uint64_t n = 0;
  for (const auto& g : groups) n += g.totalBytes();
  return n;
}

uint64_t ScanReport::selectedBytes() const {
  uint64_t n = 0;
  for (const auto& g : groups) n += g.selectedBytes();
  return n;
}

std::vector<std::string> ScanReport::selectedPaths() const {
  std::vector<std::string> out;
  for (const auto& g : groups)
    for (const auto& i : g.items)
      if (i.selected) out.push_back(i.path);
  return out;
}

void ScanReport::sortBySizeDescending() {
  for (auto& g : groups) g.sortBySizeDescending();
  std::sort(groups.begin(), groups.end(), [](const ScanGroup& a, const ScanGroup& b) {
    const uint64_t ba = a.totalBytes();
    const uint64_t bb = b.totalBytes();
    if (ba != bb) return ba > bb;
    return a.title < b.title;
  });
}

}  // namespace dcmm
