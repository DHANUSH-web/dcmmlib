#include "dcmm/path.hpp"

#include <gtest/gtest.h>

TEST(Format, Bytes) {
  EXPECT_EQ(dcmm::formatBytes(0), "0 B");
  EXPECT_EQ(dcmm::formatBytes(512), "512 B");
  EXPECT_EQ(dcmm::formatBytes(1024), "1.0 KB");
  EXPECT_EQ(dcmm::formatBytes(1024ull * 1024ull), "1.0 MB");
}

TEST(Format, Count) {
  EXPECT_EQ(dcmm::formatCount(1, "file", "files"), "1 file");
  EXPECT_EQ(dcmm::formatCount(2, "file", "files"), "2 files");
}
