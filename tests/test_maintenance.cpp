#include "dcmm/engine.hpp"
#include "dcmm/path.hpp"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class TrashEnv : public ::testing::Test {
 protected:
  fs::path home;
  void SetUp() override {
    auto base = fs::weakly_canonical(fs::temp_directory_path());
    home = base / ("dcmm-trash-" + std::to_string(reinterpret_cast<uintptr_t>(this)));
    fs::remove_all(home);
    fs::create_directories(home / ".Trash");
    fs::create_directories(home / "Documents");
    setenv("DCMM_HOME", home.string().c_str(), 1);
  }
  void TearDown() override {
    unsetenv("DCMM_HOME");
    std::error_code ec;
    fs::remove_all(home, ec);
  }
};

TEST_F(TrashEnv, EmptyPreviewWhenTrashIsEmpty) {
  dcmm::Engine e;
  auto p = e.previewMaintenance("empty_trash");
  EXPECT_TRUE(p.nothingToDo);
  EXPECT_NE(p.message.find("Nothing to clean"), std::string::npos);
}

TEST_F(TrashEnv, PreviewIgnoresFinderMetadataInTrash) {
  std::ofstream(home / ".Trash" / ".DS_Store") << "meta";
  std::ofstream(home / ".Trash" / ".localized") << "";
  dcmm::Engine e;
  auto p = e.previewMaintenance("empty_trash");
  EXPECT_TRUE(p.nothingToDo);
}

TEST_F(TrashEnv, PreviewSeesFilesInHomeTrash) {
  std::ofstream(home / ".Trash" / "a.txt") << "hello trash";
  dcmm::Engine e;
  auto p = e.previewMaintenance("empty_trash");
  EXPECT_FALSE(p.nothingToDo);
  EXPECT_GE(p.itemsAffected, 1u);
}

TEST_F(TrashEnv, EmptyRemovesTrashNotDocuments) {
  std::ofstream(home / ".Trash" / "gone.txt") << "x";
  std::ofstream(home / "Documents" / "keep.txt") << "y";
  dcmm::Engine e;
  auto r = e.runMaintenance("empty_trash");
  EXPECT_FALSE(r.nothingToDo);
  EXPECT_FALSE(fs::exists(home / ".Trash" / "gone.txt"));
  EXPECT_TRUE(fs::exists(home / "Documents" / "keep.txt"));
}
