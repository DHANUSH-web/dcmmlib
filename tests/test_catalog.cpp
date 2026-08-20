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

TEST_F(CatalogEnv, JunkItemsSortedLargestFirst) {
  fs::create_directories(home / "Library" / "Caches" / "tiny");
  std::ofstream((home / "Library" / "Caches" / "tiny" / "t.bin").string(), std::ios::binary)
      << std::string(64, 't');
  fs::create_directories(home / "Library" / "Caches" / "huge");
  std::ofstream((home / "Library" / "Caches" / "huge" / "h.bin").string(), std::ios::binary)
      << std::string(4096, 'h');
  dcmm::Engine e;
  auto r = e.scanJunk();
  const dcmm::ScanGroup* caches = nullptr;
  for (const auto& g : r.groups)
    if (g.id == "user_caches") caches = &g;
  ASSERT_NE(caches, nullptr);
  ASSERT_GE(caches->items.size(), 2u);
  for (std::size_t i = 1; i < caches->items.size(); ++i)
    EXPECT_GE(caches->items[i - 1].bytes, caches->items[i].bytes);
  EXPECT_EQ(caches->items.front().displayName, "huge");
}

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

TEST_F(CatalogEnv, SmartRecommendsCachesAsOneGroup) {
  dcmm::Engine e;
  auto r = e.scanSmart();
  EXPECT_GE(r.totalBytes(), 2048u);
  bool foundCaches = false;
  bool foundChildName = false;
  bool foundNpm = false;
  for (const auto& g : r.groups) {
    EXPECT_EQ(g.items.size(), 1u);
    EXPECT_TRUE(g.items[0].selected);
    if (g.id == "user_caches") {
      foundCaches = true;
      EXPECT_EQ(g.items[0].displayName, "User Caches");
    }
    for (const auto& it : g.items) {
      if (it.displayName == "com.example.Junk") foundChildName = true;
      if (it.path.find(".npm") != std::string::npos) foundNpm = true;
    }
  }
  EXPECT_TRUE(foundCaches);
  EXPECT_FALSE(foundChildName);
  EXPECT_FALSE(foundNpm);
}

TEST_F(CatalogEnv, TrashCachesRootMovesChildrenNotFolder) {
  auto trash = fs::temp_directory_path() / "dcmm-catalog-trash";
  fs::remove_all(trash);
  fs::create_directories(trash);
  setenv("DCMM_TRASH", trash.string().c_str(), 1);
  auto caches = home / "Library" / "Caches";
  auto child = caches / "com.example.Junk";
  ASSERT_TRUE(fs::exists(child));
  dcmm::Engine e;
  auto result = e.trashPaths({caches.string()});
  EXPECT_GE(result.trashedItems, 1u);
  EXPECT_FALSE(fs::exists(child));
  EXPECT_TRUE(fs::exists(caches));
  unsetenv("DCMM_TRASH");
  fs::remove_all(trash);
}

TEST_F(CatalogEnv, CAbiScan) {
  // covered in test_c_abi with the same env
  SUCCEED();
}
