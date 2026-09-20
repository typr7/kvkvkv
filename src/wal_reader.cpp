#include <cerrno>
#include <cassert>
#include <optional>
#include <system_error>
#include <fcntl.h>
#include <tuple>
#include <unistd.h>
#include <string_view>
#include <utility>

#include "kv/wal_reader.h"
#include "kv/wal_frame.h"
#include "kv/coding.h"
#include "kv/crc32c.h"
#include "kv/wal_record.h"


namespace kv {

namespace {

struct ReadResult {
  std::size_t bytes = 0;
  std::error_code error;
};

ReadResult ReadExactly(int fd, char* dst, std::size_t length) {
  std::size_t bytes_read = 0;

  while (bytes_read < length) {
    const auto step_read = ::read(fd, dst + bytes_read, length - bytes_read);

    if (step_read > 0) {
      bytes_read += static_cast<std::size_t>(step_read);
      continue;
    }

    if (step_read == 0) {
      return {bytes_read, {}};
    }

    const auto err = errno;

    if (err == EINTR) {
      continue;
    }

    return {bytes_read, std::error_code(err, std::generic_category())};
  }

  return {bytes_read, {}};
}

} // namespace

WalReader::~WalReader() {
  std::ignore = Close();
}

std::error_code WalReader::Open(const std::string& path) {
  if (fd_ != -1) {
    return std::make_error_code(std::errc::device_or_resource_busy);
  }

  do {
    int ret = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
    if (ret != -1) {
      terminal_ = std::nullopt;
      fd_ = ret;
      valid_bytes_ = 0;
    } else {
      const auto err = errno;
      if (terminal_) {
        terminal_->error.assign(err, std::generic_category());
      } else {
        terminal_ = {{WalReadStatus::kIoError, std::error_code(err, std::generic_category())}};
      }
    }
  } while (terminal_ && terminal_->error.value() == EINTR);

  return (terminal_ ? terminal_->error : std::error_code{});
}

WalReadResult WalReader::Next(WalRecord& output) {
  if (fd_ == -1) {
    return {WalReadStatus::kIoError, std::make_error_code(std::errc::bad_file_descriptor)};
  }

  if (terminal_) {
    return *terminal_;
  }

  // 1. read wal frame header
  std::string buffer(kWalFrameHeaderSize, '\0');

  ReadResult result = ReadExactly(fd_, buffer.data(), kWalFrameHeaderSize);

  if (result.bytes != kWalFrameHeaderSize) {
    if (result.error) {
      terminal_ = {{WalReadStatus::kIoError, result.error}};
    } else if (result.bytes == 0) {
      terminal_ = {{WalReadStatus::kEof, {}}};
    } else {
      terminal_ = {{WalReadStatus::kTruncated, {}}};
    }
    return *terminal_;
  }

  std::string_view header_view = buffer;

  // TODO: clear overlap code with DecodeWalFrame in wal_frame.cpp
  std::uint32_t payload_len;
  std::uint32_t payload_crc;
  std::uint32_t header_crc;
  std::ignore = GetFixed32(header_view, payload_len);
  std::ignore = GetFixed32(header_view, payload_crc);
  std::ignore = GetFixed32(header_view, header_crc);

  if (header_crc != Crc32c(std::string_view(buffer.begin(), buffer.begin() + 8))
      || payload_len < kWalPayloadHeaderSize
      || payload_len > kMaxWalPayloadSize) {
    terminal_ = {{WalReadStatus::kCorruption, {}}};
    return *terminal_;
  }

  // 2. read wal payload
  // resize 会保留字符串内容，如果缩小就会截断原字符串
  buffer.resize(kWalFrameHeaderSize + payload_len);

  result = ReadExactly(fd_, buffer.data() + kWalFrameHeaderSize, payload_len);

  if (result.bytes != payload_len) {
    if (result.error) {
      terminal_ = {{WalReadStatus::kIoError, result.error}};
    } else {
      terminal_ = {{WalReadStatus::kTruncated, {}}};
    }
    return *terminal_;
  }

  // 3. decode whole wal frame
  // std::string::resize() 后，指向 std::string 对象的
  // std::string_view 不一定有效，所以不可使用 header_view
  std::string_view frame_view = buffer;
  
  // DecodeWalFrame 保证了成功才会修改 output
  WalDecodeStatus status = DecodeWalFrame(frame_view, output);
  assert(status == WalDecodeStatus::kOk || status == WalDecodeStatus::kCorruption);

  if (status == WalDecodeStatus::kCorruption) {
    terminal_ = {{WalReadStatus::kCorruption, {}}};
    return *terminal_;
  }

  valid_bytes_ += buffer.size();

  return {WalReadStatus::kRecord, {}};
}

std::error_code WalReader::Close() {
  if (fd_ == -1) {
    return {};
  }

  int fd = fd_;
  fd_ = -1;

  if (::close(fd) == -1) {
    const auto err = errno;
    terminal_ = std::make_optional<WalReadResult>(WalReadStatus::kIoError,
                                                  std::error_code(err, std::generic_category()));
    return terminal_->error;
  }

  return {};
}

}