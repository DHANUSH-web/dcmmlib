#include "dcmm/engine.hpp"

#include <gtest/gtest.h>

TEST(Disk, VolumeHasCapacity) {
  dcmm::Engine e;
  auto d = e.disk("/");
  EXPECT_GT(d.totalBytes, 0u);
  EXPECT_LE(d.availableBytes, d.totalBytes);
}

TEST(Disk, MemoryHasCapacity) {
  dcmm::Engine e;
  auto m = e.memory();
  EXPECT_GT(m.totalBytes, 0u);
}
