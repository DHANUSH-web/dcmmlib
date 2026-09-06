#include "dcmm/path.hpp"
#include "dcmm/walk.hpp"

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

TEST(Walk, ContinuesAfterUnreadableSubdir) {
#if defined(_WIN32)
  GTEST_SKIP() << "chmod 000 is not a reliable permission barrier on Windows";
#else
  auto root = fs::temp_directory_path() / "dcmm-walk-unreadable";
  fs::remove_all(root);
  fs::create_directories(root / "locked");
  {
    std::ofstream((root / "early.bin").string()) << std::string(100, 'a');
    std::ofstream((root / "locked" / "secret.bin").string()) << std::string(400, 's');
    std::ofstream((root / "late.bin").string()) << std::string(200, 'b');
  }
  fs::permissions(root / "locked", fs::perms::none);
  auto sc = dcmm::directorySize(root.string());
  fs::permissions(root / "locked", fs::perms::owner_all);
  EXPECT_EQ(sc.files, 2u);
  EXPECT_EQ(sc.bytes, 300u);
  fs::remove_all(root);
#endif
}

TEST(Walk, ParentAllocatedAtLeastSumOfChildren) {
  auto root = fs::temp_directory_path() / "dcmm-walk-parent";
  fs::remove_all(root);
  fs::create_directories(root / "a");
  fs::create_directories(root / "b");
  {
    std::ofstream((root / "a" / "x.bin").string()) << std::string(1000, 'x');
    std::ofstream((root / "b" / "y.bin").string()) << std::string(2000, 'y');
    std::ofstream((root / "z.bin").string()) << std::string(100, 'z');
  }
  auto parent = dcmm::directoryAllocatedSize(root.string());
  uint64_t childSum = 0;
  childSum += dcmm::directoryAllocatedSize((root / "a").string()).bytes;
  childSum += dcmm::directoryAllocatedSize((root / "b").string()).bytes;
  childSum += dcmm::directoryAllocatedSize((root / "z.bin").string()).bytes;
  EXPECT_GE(parent.bytes, childSum);
  EXPECT_EQ(parent.bytes, childSum);
  fs::remove_all(root);
}

TEST(Walk, AllocatedSizeDoesNotExceedPaddedLogical) {
  auto root = fs::temp_directory_path() / "dcmm-walk-alloc";
  fs::remove_all(root);
  fs::create_directories(root);
  {
    std::ofstream((root / "a.bin").string()) << std::string(100, 'x');
  }
  auto logical = dcmm::directorySize(root.string());
  auto alloc = dcmm::directoryAllocatedSize(root.string());
  EXPECT_EQ(logical.files, 1u);
  EXPECT_GE(alloc.bytes, logical.bytes);
  EXPECT_LE(alloc.bytes, 16u * 1024u);
  fs::remove_all(root);
}
