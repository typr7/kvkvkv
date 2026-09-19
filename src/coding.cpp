#include <type_traits>

#include "kv/coding.h"


namespace kv {

namespace {

template <typename VT>
requires (std::is_same_v<VT, std::uint32_t> || std::is_same_v<VT, std::uint64_t>)
void PutFixed(std::string& dst, VT value) {
  constexpr int kNumBytes = sizeof(VT);
  for (int i = 0; i < kNumBytes; i++) {
    const auto byte = (value >> (i * 8)) & 0xffu;
    dst.push_back(static_cast<char>(byte));
  }
}

template <typename VT>
requires (std::is_same_v<VT, std::uint32_t> || std::is_same_v<VT, std::uint64_t>)
bool GetFixed(std::string_view& input, VT& value) {
  constexpr int kNumBytes = sizeof(VT);

  if (input.size() < kNumBytes) {
    return false;
  }

  VT decoded = 0;
  for (int i = 0; i < kNumBytes; i++) {
    // 如果 char 底层是有符号数，值为 0xff 这种直接 cast 为 uint32_t 会得到 0xffffffff，而不是 0x000000ff
    const auto byte = static_cast<VT>(static_cast<std::uint8_t>(input[i]));
    decoded |= byte << (i * 8);
  }

  value = decoded;
  input.remove_prefix(kNumBytes);
  
  return true;
}

}

void PutFixed32(std::string& dst, std::uint32_t value) {
  PutFixed(dst, value);
}

void PutFixed64(std::string& dst, std::uint64_t value) {
  PutFixed(dst, value);
}

bool GetFixed32(std::string_view& input, std::uint32_t& value) noexcept {
  return GetFixed(input, value);
}

bool GetFixed64(std::string_view& input, std::uint64_t& value) noexcept {
  return GetFixed(input, value);
}


}