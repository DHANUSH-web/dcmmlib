#include "dcmm/engine.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

TEST(Duplicates, FindsCopies) {
  auto root = fs::temp_directory_path() / "dcmm-dup-test";
  fs::remove_all(root);
  fs::create_directories(root);
  const std::string payload(4096, 'D');
  std::ofstream((root / "a.bin").string()) << payload;
  std::ofstream((root / "b.bin").string()) << payload;
  std::ofstream((root / "c.bin").string()) << std::string(4096, 'C');

  dcmm::Engine e;
  dcmm::DuplicateOptions opt;
  opt.roots = {root.string()};
  opt.minBytes = 100;
  auto groups = e.findDuplicates(opt);
  ASSERT_FALSE(groups.empty());
  bool pair = false;
  for (const auto& g : groups)
    if (g.files.size() == 2) pair = true;
  EXPECT_TRUE(pair);
  fs::remove_all(root);
}
