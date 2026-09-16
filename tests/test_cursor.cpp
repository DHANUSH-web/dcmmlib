#include "dcmm/cursor.hpp"
#include "dcmm/engine.hpp"
#include "env.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class CursorHome : public ::testing::Test {
 protected:
  fs::path home;
  void SetUp() override {
    home = fs::temp_directory_path() / "dcmm-cursor-home";
    fs::remove_all(home);
    fs::create_directories(home);
    setenv("DCMM_HOME", home.string().c_str(), 1);
  }
  void TearDown() override {
    unsetenv("DCMM_HOME");
    fs::remove_all(home);
  }
};

TEST_F(CursorHome, KnownRoots) {
  auto roots = dcmm::cursorKnownRoots(home.string());
  ASSERT_EQ(roots.size(), 5u);
  EXPECT_EQ(roots[0], (home / ".cursor").string());
  EXPECT_EQ(roots[1], (home / ".cursor-shared").string());
  EXPECT_EQ(roots[2], (home / ".cursor-server").string());
  EXPECT_EQ(roots[3], (home / ".cursor-tutor").string());
  EXPECT_EQ(roots[4], (home / "Library/Application Support/Cursor").string());
  EXPECT_EQ(dcmm::cursorExtensionsPath(home.string()), (home / ".cursor/extensions").string());
}

TEST_F(CursorHome, OmitsMissingPaths) {
  fs::create_directories(home / ".cursor");
  auto items = dcmm::cursorItems(home.string());
  ASSERT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].id, "dot_cursor");
  ASSERT_EQ(dcmm::cursorInstalls(home.string()).size(), 1u);
}

TEST_F(CursorHome, ExtensionsFirstWhenPresent) {
  fs::create_directories(home / ".cursor" / "extensions" / "pub.ext-1.0.0");
  fs::create_directories(home / ".cursor-shared");
  std::ofstream((home / ".cursor" / "extensions" / "pub.ext-1.0.0" / "package.json").string())
      << R"({"displayName":"Pub Ext"})";
  std::ofstream((home / ".cursor" / "extensions" / "pub.ext-1.0.0" / "icon.png").string()) << "png";
  auto items = dcmm::cursorItems(home.string());
  ASSERT_GE(items.size(), 2u);
  EXPECT_TRUE(items[0].extensions);
  EXPECT_EQ(items[0].label, "Extensions");
  auto exts = dcmm::cursorExtensions(home.string());
  ASSERT_EQ(exts.size(), 1u);
  EXPECT_EQ(exts[0].name, "Pub Ext");
  EXPECT_FALSE(exts[0].iconPath.empty());
}

TEST_F(CursorHome, EmptyWhenNothingInstalled) {
  EXPECT_TRUE(dcmm::cursorInstalls(home.string()).empty());
  EXPECT_TRUE(dcmm::cursorNukePaths(home.string()).empty());
}

TEST_F(CursorHome, EngineListMatchesHelpers) {
  fs::create_directories(home / ".cursor-server");
  dcmm::Engine e;
  auto inst = e.listCursor();
  ASSERT_EQ(inst.size(), 1u);
  EXPECT_EQ(inst[0].displayName, "Cursor");
}
