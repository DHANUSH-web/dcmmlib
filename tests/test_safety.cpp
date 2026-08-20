#include "dcmm/path.hpp"
#include "dcmm/safety.hpp"

#include <gtest/gtest.h>

TEST(Safety, RootAndHomeAreProtected) {
  EXPECT_TRUE(dcmm::isProtectedPath("/"));
  EXPECT_TRUE(dcmm::isProtectedPath(dcmm::homeDirectory()));
  EXPECT_FALSE(dcmm::isSafeToTrash("/"));
  EXPECT_FALSE(dcmm::isSafeToTrash("/System"));
}

TEST(Safety, DocumentsFolderProtectedNotRandomCache) {
  auto docs = dcmm::joinPath(dcmm::homeDirectory(), "Documents");
  EXPECT_TRUE(dcmm::isProtectedPath(docs));
}

TEST(Safety, BrowserNames) {
  EXPECT_TRUE(dcmm::isBrowserCacheName("com.google.Chrome"));
  EXPECT_TRUE(dcmm::isBrowserCacheName("com.apple.Safari"));
  EXPECT_FALSE(dcmm::isBrowserCacheName("com.example.MyApp"));
}
