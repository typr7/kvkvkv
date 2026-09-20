#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <system_error>

#include "memtable.h"

namespace kv {

enum class RecoveryStatus : std::uint8_t {
  kOk = 0,
  kTruncated = 1,
  kCorruption = 2,
  kIoError = 3
};

struct RecoveryResult {
  RecoveryStatus status = RecoveryStatus::kIoError;
  std::error_code error;

  std::uint64_t last_sequence = 0;
  std::uint64_t valid_bytes = 0;

  std::unique_ptr<MemTable> table;
};

[[nodiscard]]
RecoveryResult RecoverWal(const std::string& path);

}