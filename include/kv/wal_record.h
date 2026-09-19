#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "internal_key.h"


namespace kv {

// WAL memory layout
// [sequence: uint64_t][type: 1 byte][key_length: uint32_t][value_length: uint32_t][key][value]

inline constexpr std::size_t kWalPayloadHeaderSize = 17;
inline constexpr std::size_t kMaxWalPayloadSize = 16 * 1024 * 1024; // 16 MiB

struct WalRecord {
  std::uint64_t sequence = 0;
  ValueType type = ValueType::kValue;
  std::string key;
  std::string value;
};

[[nodiscard]]
bool EncodeWalPayload(const WalRecord& record, std::string& output);

[[nodiscard]]
bool DecodeWalPayload(std::string_view input, WalRecord& output);

}