#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(DCMM_STATIC)
#define DCMM_API
#elif defined(_WIN32) || defined(_WIN64)
#if defined(DCMM_EXPORTS)
#define DCMM_API __declspec(dllexport)
#else
#define DCMM_API __declspec(dllimport)
#endif
#else
#define DCMM_API __attribute__((visibility("default")))
#endif

typedef struct dcmm_engine dcmm_engine;

typedef void (*dcmm_progress_fn)(const char* path, uint64_t visited, uint64_t bytes, void* user);

typedef struct dcmm_clean_result {
  uint64_t trashed_bytes;
  uint64_t trashed_items;
  uint64_t failed_items;
} dcmm_clean_result;

typedef struct dcmm_disk_stats {
  uint64_t total_bytes;
  uint64_t available_bytes;
  uint64_t free_bytes;
  char volume_name[128];
  char mount_point[512];
} dcmm_disk_stats;

typedef struct dcmm_memory_stats {
  uint64_t total_bytes;
  uint64_t used_bytes;
  uint64_t free_bytes;
} dcmm_memory_stats;

DCMM_API const char* dcmm_version(void);

DCMM_API dcmm_engine* dcmm_create(void);
DCMM_API void dcmm_destroy(dcmm_engine* engine);
DCMM_API void dcmm_cancel(dcmm_engine* engine);

DCMM_API int dcmm_scan_junk(dcmm_engine* engine, dcmm_progress_fn fn, void* user);
DCMM_API int dcmm_scan_privacy(dcmm_engine* engine, dcmm_progress_fn fn, void* user);

DCMM_API uint64_t dcmm_scan_total_bytes(const dcmm_engine* engine);
DCMM_API uint64_t dcmm_scan_elapsed_ms(const dcmm_engine* engine);
DCMM_API size_t dcmm_group_count(const dcmm_engine* engine);
DCMM_API const char* dcmm_group_id(const dcmm_engine* engine, size_t group);
DCMM_API const char* dcmm_group_title(const dcmm_engine* engine, size_t group);
DCMM_API const char* dcmm_group_subtitle(const dcmm_engine* engine, size_t group);
DCMM_API uint64_t dcmm_group_bytes(const dcmm_engine* engine, size_t group);
DCMM_API size_t dcmm_item_count(const dcmm_engine* engine, size_t group);
DCMM_API const char* dcmm_item_path(const dcmm_engine* engine, size_t group, size_t item);
DCMM_API const char* dcmm_item_name(const dcmm_engine* engine, size_t group, size_t item);
DCMM_API uint64_t dcmm_item_bytes(const dcmm_engine* engine, size_t group, size_t item);
DCMM_API uint64_t dcmm_item_files(const dcmm_engine* engine, size_t group, size_t item);
DCMM_API int dcmm_item_selected(const dcmm_engine* engine, size_t group, size_t item);
DCMM_API void dcmm_item_set_selected(dcmm_engine* engine, size_t group, size_t item, int selected);

DCMM_API int dcmm_clean_selected(dcmm_engine* engine, dcmm_clean_result* out);
DCMM_API int dcmm_trash_paths(const char* const* paths, size_t n, dcmm_clean_result* out);

DCMM_API int dcmm_disk(const char* path, dcmm_disk_stats* out);
DCMM_API int dcmm_memory(dcmm_memory_stats* out);

DCMM_API char* dcmm_format_bytes(uint64_t bytes);
DCMM_API void dcmm_string_free(char* s);

#ifdef __cplusplus
}
#endif
