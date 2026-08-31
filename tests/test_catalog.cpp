#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"
#include "env.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

fs::path userCacheRoot(const fs::path& home) {
#if defined(__APPLE__)
  return home / "Library" / "Caches";
#elif defined(_WIN32)
  return home / "AppData" / "Local" / "Temp";
#else
  return home / ".cache";
#endif
}

const char* userCacheGroupId() {
#if defined(__APPLE__)
  return "user_caches";
#elif defined(_WIN32)
  return "temp";
#else
  return "user_cache";
#endif
}

const char* userCacheGroupTitle() {
#if defined(__APPLE__)
  return "User Caches";
#elif defined(_WIN32)
  return "User Temp";
#else
  return "User Cache";
#endif
}

class CatalogEnv : public ::testing::Test {
 protected:
  fs::path home;
  void SetUp() override {
    home = fs::temp_directory_path() / "dcmm-catalog-home";
    fs::remove_all(home);
    auto caches = userCacheRoot(home);
    fs::create_directories(caches / "com.example.Junk");
    std::ofstream((caches / "com.example.Junk" / "c.bin").string()) << std::string(2048, 'j');
    // Category-root trash test uses Library/Caches on every OS.
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
  auto cachesDir = userCacheRoot(home);
  fs::create_directories(cachesDir / "tiny");
  std::ofstream((cachesDir / "tiny" / "t.bin").string(), std::ios::binary) << std::string(64, 't');
  fs::create_directories(cachesDir / "huge");
  std::ofstream((cachesDir / "huge" / "h.bin").string(), std::ios::binary) << std::string(4096, 'h');
  dcmm::Engine e;
  auto r = e.scanJunk();
  const dcmm::ScanGroup* caches = nullptr;
  for (const auto& g : r.groups)
    if (g.id == userCacheGroupId()) caches = &g;
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
    if (g.id != "installers") EXPECT_EQ(g.items.size(), 1u);
    EXPECT_TRUE(g.items[0].selected);
    if (g.id == userCacheGroupId()) {
      foundCaches = true;
      EXPECT_EQ(g.items[0].displayName, userCacheGroupTitle());
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

TEST_F(CatalogEnv, SmartFindsInstallerLeftovers) {
  fs::create_directories(home / "Downloads");
  fs::create_directories(home / "Documents");
  std::ofstream((home / "Downloads" / "App.dmg").string(), std::ios::binary) << std::string(1024, 'd');
  std::ofstream((home / "Documents" / "Setup.pkg").string(), std::ios::binary) << std::string(512, 'p');
  std::ofstream((home / "Downloads" / "notes.txt").string()) << "no";
  dcmm::Engine e;
  auto r = e.scanSmart();
  const dcmm::ScanGroup* installers = nullptr;
  for (const auto& g : r.groups)
    if (g.id == "installers") installers = &g;
  ASSERT_NE(installers, nullptr);
  bool dmg = false, pkg = false, txt = false;
  for (const auto& it : installers->items) {
    EXPECT_TRUE(it.selected);
    if (it.displayName == "App.dmg") dmg = true;
    if (it.displayName == "Setup.pkg") pkg = true;
    if (it.displayName == "notes.txt") txt = true;
  }
  EXPECT_TRUE(dmg);
  EXPECT_TRUE(pkg);
  EXPECT_FALSE(txt);
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
