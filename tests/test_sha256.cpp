#include "dcmm/sha256.hpp"

#include <gtest/gtest.h>

TEST(Sha256, Empty) {
  uint8_t d[dcmm::kSha256Size];
  dcmm::sha256Bytes("", 0, d);
  EXPECT_EQ(dcmm::sha256Hex(d),
            "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(Sha256, Abc) {
  uint8_t d[dcmm::kSha256Size];
  const char* s = "abc";
  dcmm::sha256Bytes(s, 3, d);
  EXPECT_EQ(dcmm::sha256Hex(d),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}
