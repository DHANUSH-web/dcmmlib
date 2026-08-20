#include "dcmm/dcmm.h"

#include <gtest/gtest.h>

TEST(CAbi, VersionAndFormat) {
  EXPECT_STREQ(dcmm_version(), "1.0.0");
  char* s = dcmm_format_bytes(1024);
  ASSERT_NE(s, nullptr);
  EXPECT_STREQ(s, "1.0 KB");
  dcmm_string_free(s);
}

TEST(CAbi, CreateDestroyDisk) {
  dcmm_engine* e = dcmm_create();
  ASSERT_NE(e, nullptr);
  dcmm_disk_stats d{};
  ASSERT_EQ(dcmm_disk("/", &d), 0);
  EXPECT_GT(d.total_bytes, 0u);
  dcmm_memory_stats m{};
  ASSERT_EQ(dcmm_memory(&m), 0);
  EXPECT_GT(m.total_bytes, 0u);
  dcmm_destroy(e);
}

TEST(CAbi, TrashRejectsRoot) {
  const char* paths[] = {"/"};
  dcmm_clean_result r{};
  int rc = dcmm_trash_paths(paths, 1, &r);
  EXPECT_NE(rc, 0);
  EXPECT_GE(r.failed_items, 1u);
  EXPECT_EQ(r.trashed_items, 0u);
}
