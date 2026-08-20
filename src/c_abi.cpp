#include "dcmm/dcmm.h"

#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#ifndef DCMM_EXPORTS
#define DCMM_EXPORTS
#endif

struct dcmm_engine {
  dcmm::Engine impl;
};

extern "C" {

const char* dcmm_version(void) { return "1.0.0"; }

dcmm_engine* dcmm_create(void) {
  try {
    return new dcmm_engine();
  } catch (...) {
    return nullptr;
  }
}

void dcmm_destroy(dcmm_engine* engine) { delete engine; }

void dcmm_cancel(dcmm_engine* engine) {
  if (engine) engine->impl.cancel();
}

static int runScan(dcmm_engine* engine, dcmm_progress_fn fn, void* user, int kind) {
  if (!engine) return -1;
  dcmm::ProgressFn cb;
  if (fn) {
    cb = [fn, user](const std::string& p, uint64_t v, uint64_t b) { fn(p.c_str(), v, b, user); };
  }
  if (kind == 1)
    engine->impl.scanPrivacy(cb);
  else if (kind == 2)
    engine->impl.scanSmart(cb);
  else
    engine->impl.scanJunk(cb);
  return 0;
}

int dcmm_scan_smart(dcmm_engine* engine, dcmm_progress_fn fn, void* user) {
  return runScan(engine, fn, user, 2);
}
int dcmm_scan_junk(dcmm_engine* engine, dcmm_progress_fn fn, void* user) {
  return runScan(engine, fn, user, 0);
}
int dcmm_scan_privacy(dcmm_engine* engine, dcmm_progress_fn fn, void* user) {
  return runScan(engine, fn, user, 1);
}

static const dcmm::ScanReport* report(const dcmm_engine* e) {
  return e ? &e->impl.lastScan() : nullptr;
}

uint64_t dcmm_scan_total_bytes(const dcmm_engine* engine) {
  auto r = report(engine);
  return r ? r->totalBytes() : 0;
}
uint64_t dcmm_scan_elapsed_ms(const dcmm_engine* engine) {
  auto r = report(engine);
  return r ? r->elapsedMs : 0;
}
size_t dcmm_group_count(const dcmm_engine* engine) {
  auto r = report(engine);
  return r ? r->groups.size() : 0;
}

static const dcmm::ScanGroup* groupAt(const dcmm_engine* e, size_t g) {
  auto r = report(e);
  if (!r || g >= r->groups.size()) return nullptr;
  return &r->groups[g];
}
static const dcmm::ScanItem* itemAt(const dcmm_engine* e, size_t g, size_t i) {
  auto grp = groupAt(e, g);
  if (!grp || i >= grp->items.size()) return nullptr;
  return &grp->items[i];
}

const char* dcmm_group_id(const dcmm_engine* e, size_t g) {
  auto grp = groupAt(e, g);
  return grp ? grp->id.c_str() : "";
}
const char* dcmm_group_title(const dcmm_engine* e, size_t g) {
  auto grp = groupAt(e, g);
  return grp ? grp->title.c_str() : "";
}
const char* dcmm_group_subtitle(const dcmm_engine* e, size_t g) {
  auto grp = groupAt(e, g);
  return grp ? grp->subtitle.c_str() : "";
}
uint64_t dcmm_group_bytes(const dcmm_engine* e, size_t g) {
  auto grp = groupAt(e, g);
  return grp ? grp->totalBytes() : 0;
}
size_t dcmm_item_count(const dcmm_engine* e, size_t g) {
  auto grp = groupAt(e, g);
  return grp ? grp->items.size() : 0;
}
const char* dcmm_item_path(const dcmm_engine* e, size_t g, size_t i) {
  auto it = itemAt(e, g, i);
  return it ? it->path.c_str() : "";
}
const char* dcmm_item_name(const dcmm_engine* e, size_t g, size_t i) {
  auto it = itemAt(e, g, i);
  return it ? it->displayName.c_str() : "";
}
uint64_t dcmm_item_bytes(const dcmm_engine* e, size_t g, size_t i) {
  auto it = itemAt(e, g, i);
  return it ? it->bytes : 0;
}
uint64_t dcmm_item_files(const dcmm_engine* e, size_t g, size_t i) {
  auto it = itemAt(e, g, i);
  return it ? it->fileCount : 0;
}
int dcmm_item_selected(const dcmm_engine* e, size_t g, size_t i) {
  auto it = itemAt(e, g, i);
  return it && it->selected ? 1 : 0;
}
void dcmm_item_set_selected(dcmm_engine* e, size_t g, size_t i, int selected) {
  if (!e) return;
  auto& groups = e->impl.lastScan().groups;
  if (g >= groups.size() || i >= groups[g].items.size()) return;
  groups[g].items[i].selected = selected != 0;
}

int dcmm_clean_selected(dcmm_engine* engine, dcmm_clean_result* out) {
  if (!engine) return -1;
  auto r = engine->impl.trashSelected();
  if (out) {
    out->trashed_bytes = r.trashedBytes;
    out->trashed_items = r.trashedItems;
    out->failed_items = r.failedItems;
  }
  return r.failedItems ? 1 : 0;
}

int dcmm_trash_paths(const char* const* paths, size_t n, dcmm_clean_result* out) {
  dcmm::Engine e;
  std::vector<std::string> v;
  v.reserve(n);
  for (size_t i = 0; i < n; ++i)
    if (paths && paths[i]) v.emplace_back(paths[i]);
  auto r = e.trashPaths(v);
  if (out) {
    out->trashed_bytes = r.trashedBytes;
    out->trashed_items = r.trashedItems;
    out->failed_items = r.failedItems;
  }
  return r.failedItems ? 1 : 0;
}

int dcmm_disk(const char* path, dcmm_disk_stats* out) {
  if (!out) return -1;
  dcmm::Engine e;
  auto d = e.disk(path ? path : "");
  out->total_bytes = d.totalBytes;
  out->available_bytes = d.availableBytes;
  out->free_bytes = d.freeBytes;
  std::snprintf(out->volume_name, sizeof(out->volume_name), "%s", d.volumeName.c_str());
  std::snprintf(out->mount_point, sizeof(out->mount_point), "%s", d.mountPoint.c_str());
  return 0;
}

int dcmm_memory(dcmm_memory_stats* out) {
  if (!out) return -1;
  dcmm::Engine e;
  auto m = e.memory();
  out->total_bytes = m.totalBytes;
  out->used_bytes = m.usedBytes;
  out->free_bytes = m.freeBytes;
  return 0;
}

char* dcmm_format_bytes(uint64_t bytes) {
  auto s = dcmm::formatBytes(bytes);
  char* p = static_cast<char*>(std::malloc(s.size() + 1));
  if (!p) return nullptr;
  std::memcpy(p, s.c_str(), s.size() + 1);
  return p;
}

void dcmm_string_free(char* s) { std::free(s); }

}  // extern "C"
