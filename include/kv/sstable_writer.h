#pragma once

#include <string>
#include <system_error>

#include "sstable_format.h"


namespace kv {

class SSTableWriter {
public:
  SSTableWriter() = default;
  ~SSTableWriter();

  SSTableWriter(const SSTableWriter&) = delete;
  SSTableWriter& operator=(const SSTableWriter&) = delete;
  SSTableWriter(SSTableWriter&&) = delete;
  SSTableWriter& operator=(SSTableWriter&&) = delete;

  [[nodiscard]]
  std::error_code OpenNew(const std::string& path);

  [[nodiscard]]
  std::error_code AppendBlock(
      std::string_view payload,
      BlockHandle& handle);

  [[nodiscard]]
  std::error_code Finish(const BlockHandle& index_handle);

  [[nodiscard]]
  std::error_code Close();

private:
  std::error_code WriteAll(std::string_view bytes);
  std::error_code Sync();

  int fd_ = -1;
  std::uint64_t offset_ = 0;
  std::error_code error_;
};

}