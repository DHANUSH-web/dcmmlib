#include "dcmm/path.hpp"
#include "env.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>

TEST(Path, JoinAndDisplay) {
  auto p = dcmm::joinPath("/tmp", "foo");
  EXPECT_NE(p.find("foo"), std::string::npos);
  EXPECT_EQ(dcmm::displayName("/a/b/c.txt"), "c.txt");
}

TEST(Path, ExpandHome) {
  setenv("DCMM_HOME", "/tmp/dcmm-home-test", 1);
  EXPECT_EQ(dcmm::homeDirectory(), "/tmp/dcmm-home-test");
  auto e = dcmm::expandPath("~/Library");
  EXPECT_NE(e.find("dcmm-home-test"), std::string::npos);
  unsetenv("DCMM_HOME");
}

TEST(Path, Exists) {
  EXPECT_TRUE(dcmm::pathExists("/"));
  EXPECT_TRUE(dcmm::isDirectory("/"));
  EXPECT_FALSE(dcmm::pathExists("/this/path/should/not/exist/dcmm-xyz"));
}
