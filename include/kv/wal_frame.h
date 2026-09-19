#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include "wal_record.h"


namespace kv {

// WAL layout
// [payload_length: uint32_t][payload crc: 4 bytes][header crc: 4 bytes][payload]
inline constexpr std::size_t kWalFrameHeaderSize = 12;

enum class WalDecodeStatus : std::uint8_t {
  kOk = 0, // 成功解析
  kIncomplete = 1, // 还需要继续解析
  kCorruption = 2, // 损坏
};

[[nodiscard]]
bool EncodeWalFrame(const WalRecord& record, std::string& output);

[[nodiscard]]
WalDecodeStatus DecodeWalFrame(std::string_view& input, WalRecord& output);

}