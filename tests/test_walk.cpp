#include "dcmm/path.hpp"
#include "walk.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST(Walk, DirectorySize) {
  auto root = fs::temp_directory_path() / "dcmm-walk-test";
  fs::remove_all(root);
  fs::create_directories(root / "sub");
  {
    std::ofstream((root / "a.bin").string()) << std::string(100, 'x');
    std::ofstream((root / "sub" / "b.bin").string()) << std::string(50, 'y');
  }
  auto sc = dcmm::directorySize(root.string());
  EXPECT_EQ(sc.bytes, 150u);
  EXPECT_EQ(sc.files, 2u);
  fs::remove_all(root);
}

TEST(Walk, SkipGitName) {
  EXPECT_TRUE(dcmm::skipDirectoryName(".git"));
  EXPECT_FALSE(dcmm::skipDirectoryName("src"));
}
