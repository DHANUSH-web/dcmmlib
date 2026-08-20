#include "dcmm/types.hpp"

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

}  // namespace dcmm
