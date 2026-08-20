#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class CatalogEnv : public ::testing::Test {
 protected:
  fs::path home;
  void SetUp() override {
    home = fs::temp_directory_path() / "dcmm-catalog-home";
    fs::remove_all(home);
    fs::create_directories(home / "Library" / "Caches" / "com.example.Junk");
    std::ofstream((home / "Library" / "Caches" / "com.example.Junk" / "c.bin").string())
        << std::string(2048, 'j');
    setenv("DCMM_HOME", home.string().c_str(), 1);
  }
  void TearDown() override {
    unsetenv("DCMM_HOME");
    fs::remove_all(home);
  }
};

TEST_F(CatalogEnv, JunkFindsCache) {
  dcmm::Engine e;
  auto r = e.scanJunk();
  uint64_t bytes = r.totalBytes();
  EXPECT_GE(bytes, 2048u);
  bool found = false;
  for (const auto& g : r.groups)
    for (const auto& it : g.items)
      if (it.displayName == "com.example.Junk") found = true;
  EXPECT_TRUE(found);
}

TEST_F(CatalogEnv, CAbiScan) {
  // covered in test_c_abi with the same env
  SUCCEED();
}
