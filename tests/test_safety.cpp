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

TEST(Safety, SensitiveKeysBlocked) {
  EXPECT_TRUE(dcmm::isSensitiveFileName("id_rsa"));
  EXPECT_TRUE(dcmm::isSensitiveFileName("secret.pem"));
  EXPECT_FALSE(dcmm::isSafeToTrash(dcmm::joinPath(dcmm::homeDirectory(), "Downloads/id_rsa")));
}

TEST(Safety, ApplicationSupportParentBlocked) {
  auto support = dcmm::joinPath(dcmm::homeDirectory(), "Library/Application Support");
  EXPECT_FALSE(dcmm::isSafeToTrash(support));
  EXPECT_FALSE(dcmm::isSafeToTrash(dcmm::joinPath(support, "Google")));
  EXPECT_FALSE(dcmm::isSafeToTrash(dcmm::joinPath(support, "com.apple.Safari")));
}

TEST(Safety, CacheChildAllowed) {
  auto cache = dcmm::joinPath(dcmm::joinPath(dcmm::homeDirectory(), "Library/Caches"),
                              "com.example.Junk");
  EXPECT_FALSE(dcmm::isProtectedPath(cache));
  EXPECT_TRUE(dcmm::isSafeToTrash(cache));
}

TEST(Safety, CacheFolderIsCategoryRootNotDirectlyTrashable) {
  auto caches = dcmm::joinPath(dcmm::homeDirectory(), "Library/Caches");
  EXPECT_TRUE(dcmm::isJunkCategoryRoot(caches));
  EXPECT_FALSE(dcmm::isSafeToTrash(caches));
}

TEST(Safety, SystemLibraryCachesNotACategoryRoot) {
  EXPECT_FALSE(dcmm::isJunkCategoryRoot("/Library/Caches"));
}

TEST(Safety, SshTreeBlocked) {
  EXPECT_TRUE(dcmm::isProtectedPath(dcmm::joinPath(dcmm::homeDirectory(), ".ssh/id_ed25519")));
}
