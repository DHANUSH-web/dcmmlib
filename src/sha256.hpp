#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace dcmm {

constexpr std::size_t kSha256Size = 32;

bool sha256File(const std::string& path, uint64_t maxBytes, uint8_t out[kSha256Size]);
void sha256Bytes(const void* data, std::size_t len, uint8_t out[kSha256Size]);
std::string sha256Hex(const uint8_t digest[kSha256Size]);

}  // namespace dcmm
