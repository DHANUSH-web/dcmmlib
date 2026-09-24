#include "dcmm/antigravity.hpp"
#include "dcmm/engine.hpp"
#include "env.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class AntigravityHome : public ::testing::Test {
 protected:
  fs::path home;
  void SetUp() override {
    home = fs::temp_directory_path() / "dcmm-antigravity-home";
    fs::remove_all(home);
    fs::create_directories(home);
    setenv("DCMM_HOME", home.string().c_str(), 1);
  }
  void TearDown() override {
    unsetenv("DCMM_HOME");
    fs::remove_all(home);
  }
};

TEST_F(AntigravityHome, KnownRoots) {
  auto roots = dcmm::antigravityKnownRoots(home.string());
  ASSERT_EQ(roots.size(), 5u);
  EXPECT_EQ(roots[0], (home / ".antigravity-ide").string());
  EXPECT_EQ(roots[1], (home / ".antigravity-ide-shared").string());
  EXPECT_EQ(roots[2], (home / ".antigravity-ide-server").string());
  EXPECT_EQ(roots[3], (home / ".gemini" / "antigravity-ide").string());
  EXPECT_EQ(roots[4], (home / "Library/Application Support/Antigravity IDE").string());
  EXPECT_EQ(dcmm::antigravityExtensionsPath(home.string()),
            (home / ".antigravity-ide/extensions").string());
}

TEST_F(AntigravityHome, OmitsMissingPaths) {
  fs::create_directories(home / ".antigravity-ide");
  auto items = dcmm::antigravityItems(home.string());
  ASSERT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].id, "dot_antigravity_ide");
  ASSERT_EQ(dcmm::antigravityInstalls(home.string()).size(), 1u);
}

TEST_F(AntigravityHome, ExtensionsFirstWhenPresent) {
  auto extDir = home / ".antigravity-ide" / "extensions" / "pub.ext-1.0.0";
  fs::create_directories(extDir / "media");
  fs::create_directories(home / ".antigravity-ide-server");
  std::ofstream((extDir / "package.json").string())
      << R"({"name":"pub-ext","displayName":"Pub Ext","version":"3.0.0","publisher":"pub",)"
         R"("icon":"media/logo.png","repository":{"url":"https://example.com/agy"}})";
  std::ofstream((extDir / "media" / "logo.png").string()) << "png";
  auto items = dcmm::antigravityItems(home.string());
  ASSERT_GE(items.size(), 2u);
  EXPECT_TRUE(items[0].extensions);
  EXPECT_EQ(items[0].label, "Extensions");
  auto exts = dcmm::antigravityExtensions(home.string());
  ASSERT_EQ(exts.size(), 1u);
  EXPECT_EQ(exts[0].name, "Pub Ext");
  EXPECT_EQ(exts[0].version, "3.0.0");
  EXPECT_EQ(exts[0].publisher, "pub");
  EXPECT_EQ(exts[0].repositoryUrl, "https://example.com/agy");
  EXPECT_EQ(exts[0].iconPath, (extDir / "media" / "logo.png").string());
}

TEST_F(AntigravityHome, EmptyWhenNothingInstalled) {
  EXPECT_TRUE(dcmm::antigravityInstalls(home.string()).empty());
  EXPECT_TRUE(dcmm::antigravityNukePaths(home.string()).empty());
}

TEST_F(AntigravityHome, EngineListMatchesHelpers) {
  fs::create_directories(home / ".antigravity-ide-server");
  dcmm::Engine e;
  auto inst = e.listAntigravity();
  ASSERT_EQ(inst.size(), 1u);
  EXPECT_EQ(inst[0].displayName, "Antigravity IDE");
}
