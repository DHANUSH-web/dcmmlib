#include "dcmm/engine.hpp"
#include "dcmm/vscode.hpp"
#include "env.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class VsCodeHome : public ::testing::Test {
 protected:
  fs::path home;
  void SetUp() override {
    home = fs::temp_directory_path() / "dcmm-vscode-home";
    fs::remove_all(home);
    fs::create_directories(home);
    setenv("DCMM_HOME", home.string().c_str(), 1);
  }
  void TearDown() override {
    unsetenv("DCMM_HOME");
    fs::remove_all(home);
  }
};

TEST_F(VsCodeHome, KnownRootsDifferByEdition) {
  auto st = dcmm::vsCodeKnownRoots(dcmm::VsCodeEdition::Stable, home.string());
  auto in = dcmm::vsCodeKnownRoots(dcmm::VsCodeEdition::Insiders, home.string());
  ASSERT_EQ(st.size(), 4u);
  ASSERT_EQ(in.size(), 4u);
  EXPECT_EQ(st[0], (home / ".vscode").string());
  EXPECT_EQ(st[1], (home / ".vscode-shared").string());
  EXPECT_EQ(st[2], (home / ".vscode-server").string());
  EXPECT_EQ(st[3], (home / "Library/Application Support/Code").string());
  EXPECT_EQ(in[0], (home / ".vscode-insiders").string());
  EXPECT_EQ(in[3], (home / "Library/Application Support/Code - Insiders").string());
  EXPECT_EQ(dcmm::vsCodeExtensionsPath(dcmm::VsCodeEdition::Stable, home.string()),
            (home / ".vscode/extensions").string());
}

TEST_F(VsCodeHome, OmitsMissingPaths) {
  fs::create_directories(home / ".vscode");
  auto items = dcmm::vsCodeItems(dcmm::VsCodeEdition::Stable, home.string());
  ASSERT_EQ(items.size(), 1u);
  EXPECT_EQ(items[0].id, "dot_vscode");
  EXPECT_TRUE(dcmm::vsCodeInstalls(home.string()).size() == 1u);
}

TEST_F(VsCodeHome, ExtensionsFirstWhenPresent) {
  auto extDir = home / ".vscode" / "extensions" / "pub.ext-1.0.0";
  fs::create_directories(extDir / "images");
  fs::create_directories(home / ".vscode-shared");
  {
    std::ofstream((extDir / "package.json").string())
        << R"({"name":"pub-ext","displayName":"Pub Ext","version":"1.0.0","publisher":"pub",)"
           R"("icon":"images/icon.png","repository":{"type":"git","url":"https://example.com/ext"}})";
    std::ofstream((extDir / "images" / "icon.png").string()) << "png";
    std::ofstream((extDir / "icon.png").string()) << "ignored";
  }
  auto items = dcmm::vsCodeItems(dcmm::VsCodeEdition::Stable, home.string());
  ASSERT_GE(items.size(), 2u);
  EXPECT_TRUE(items[0].extensions);
  EXPECT_EQ(items[0].label, "Extensions");
  auto exts = dcmm::vsCodeExtensions(dcmm::VsCodeEdition::Stable, home.string());
  ASSERT_EQ(exts.size(), 1u);
  EXPECT_EQ(exts[0].name, "Pub Ext");
  EXPECT_EQ(exts[0].version, "1.0.0");
  EXPECT_EQ(exts[0].publisher, "pub");
  EXPECT_EQ(exts[0].repositoryUrl, "https://example.com/ext");
  EXPECT_EQ(exts[0].iconPath, (extDir / "images" / "icon.png").string());
  EXPECT_GT(exts[0].bytes, 0u);
}

TEST_F(VsCodeHome, PlaceholderDisplayNameUsesNameThenFolder) {
  fs::create_directories(home / ".vscode" / "extensions" / "pub.thing-1.0.0");
  std::ofstream((home / ".vscode" / "extensions" / "pub.thing-1.0.0" / "package.json").string())
      << R"({"name":"pub-thing","displayName":"%displayName%"})";
  auto exts = dcmm::vsCodeExtensions(dcmm::VsCodeEdition::Stable, home.string());
  ASSERT_EQ(exts.size(), 1u);
  EXPECT_EQ(exts[0].name, "pub-thing");

  fs::create_directories(home / ".vscode" / "extensions" / "pub.other-1.0.0");
  std::ofstream((home / ".vscode" / "extensions" / "pub.other-1.0.0" / "package.json").string())
      << R"({"displayName":"%displayName%"})";
  exts = dcmm::vsCodeExtensions(dcmm::VsCodeEdition::Stable, home.string());
  ASSERT_EQ(exts.size(), 2u);
  bool sawFolder = false;
  for (const auto& e : exts)
    if (e.name == "pub.other-1.0.0") sawFolder = true;
  EXPECT_TRUE(sawFolder);
}

TEST_F(VsCodeHome, HiddenAndFilesAreNotExtensions) {
  fs::create_directories(home / ".vscode" / "extensions" / ".obsolete");
  std::ofstream((home / ".vscode" / "extensions" / "extensions.json").string()) << "{}";
  auto exts = dcmm::vsCodeExtensions(dcmm::VsCodeEdition::Stable, home.string());
  EXPECT_TRUE(exts.empty());
}

TEST_F(VsCodeHome, EmptyWhenNothingInstalled) {
  EXPECT_TRUE(dcmm::vsCodeInstalls(home.string()).empty());
  EXPECT_TRUE(dcmm::vsCodeNukePaths(dcmm::VsCodeEdition::Stable, home.string()).empty());
}

TEST_F(VsCodeHome, NukeListsExistingRootsNotMissingServer) {
  fs::create_directories(home / ".vscode");
  auto nuke = dcmm::vsCodeNukePaths(dcmm::VsCodeEdition::Stable, home.string());
  ASSERT_EQ(nuke.size(), 1u);
  EXPECT_EQ(nuke[0], (home / ".vscode").string());
}

TEST_F(VsCodeHome, EngineListMatchesHelpers) {
  fs::create_directories(home / ".vscode-insiders" / "extensions" / "a.b-1");
  dcmm::Engine e;
  auto inst = e.listVsCode();
  ASSERT_EQ(inst.size(), 1u);
  EXPECT_EQ(inst[0].edition, dcmm::VsCodeEdition::Insiders);
  EXPECT_EQ(inst[0].displayName, "Visual Studio Code - Insiders");
}
