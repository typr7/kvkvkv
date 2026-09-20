#pragma once

#include <string>
#include <system_error>

#include "wal_record.h"


namespace kv {

class WalWriter {
public:
  WalWriter() = default;
  ~WalWriter();

  WalWriter(const WalWriter&) = delete;
  WalWriter& operator=(const WalWriter&) = delete;
  WalWriter(WalWriter&&) = delete;
  WalWriter& operator=(WalWriter&&) = delete;

  [[nodiscard]]
  std::error_code OpenNew(const std::string& path);

  [[nodiscard]]
  std::error_code Append(const WalRecord& record);

  [[nodiscard]]
  std::error_code Sync();

  [[nodiscard]]
  std::error_code Close();

private:
  int fd_ = -1;
  std::error_code error_;
};

}