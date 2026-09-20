#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

#include "wal_record.h"


namespace kv {

enum class WalReadStatus : std::uint8_t {
  kRecord = 0,      // 成功读取一条记录
  kEof = 1,         // 恰好在记录边界到达文件末尾
  kTruncated = 2,   // 文件末尾存在不完整的头部或 payload
  kCorruption = 3,  // 校验不匹配、长度非法或 payload 格式非法
  kIoError = 4
};

struct WalReadResult {
  WalReadStatus status = WalReadStatus::kEof;
  std::error_code error;
};

class WalReader {
public:
  WalReader() = default;
  ~WalReader();

  WalReader(const WalReader&) = delete;
  WalReader& operator=(const WalReader&) = delete;
  WalReader(WalReader&&) = delete;
  WalReader& operator=(WalReader&&) = delete;

  [[nodiscard]]
  std::error_code Open(const std::string& path);

  [[nodiscard]]
  WalReadResult Next(WalRecord& output);

  [[nodiscard]]
  std::error_code Close();

  [[nodiscard]]
  std::uint64_t valid_bytes() const noexcept {
    return valid_bytes_;
  }

private:
  int fd_ = -1;
  std::uint64_t valid_bytes_ = 0; // 保存读取的有效 bytes (不包括损坏的与截断的 record bytes)
  std::optional<WalReadResult> terminal_;
};

}