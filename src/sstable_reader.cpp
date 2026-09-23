#include <array>
#include <cerrno>
#include <tuple>
#include <utility>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "kv/coding.h"
#include "kv/crc32c.h"
#include "kv/sstable_reader.h"


namespace kv {

SSTableReader::~SSTableReader() {
  std::ignore = Close();
}

std::error_code SSTableReader::Open(const std::string& path) {
  if (fd_ != -1) {
    return std::make_error_code(std::errc::device_or_resource_busy);
  }

  SSTableReader pending;
  do {
    pending.fd_ = ::open(path.c_str(), O_RDONLY | O_CLOEXEC);
  } while (pending.fd_ == -1 && errno == EINTR);

  if (pending.fd_ == -1) {
    return std::error_code(errno, std::generic_category());
  }

  struct stat info {};
  int ret;
  do {
    ret = ::fstat(pending.fd_, &info);
  } while (ret == -1 && errno == EINTR);

  if (ret == -1) {
    return std::error_code(errno, std::generic_category());
  }
  if (!S_ISREG(info.st_mode)) {
    return std::make_error_code(std::errc::invalid_argument);
  }
  if (info.st_size < 0 ||
      static_cast<std::uint64_t>(info.st_size) < kSSTableFooterSize + 8) {
    return std::make_error_code(std::errc::bad_message);
  }

  const auto file_size = static_cast<std::uint64_t>(info.st_size);
  const auto footer_start = file_size - kSSTableFooterSize;
  std::array<char, kSSTableFooterSize> footer;
  auto err = pending.ReadExactly(footer_start, footer);
  if (err) {
    return err;
  }

  BlockHandle index_handle;
  if (!DecodeFooter(std::string_view(footer.data(), footer.size()), index_handle)) {
    return std::make_error_code(std::errc::bad_message);
  }
  if (index_handle.size < 4 || index_handle.size > kMaxIndexBlockSize ||
      index_handle.size > footer_start - 4 ||
      index_handle.offset != footer_start - 4 - index_handle.size) {
    return std::make_error_code(std::errc::bad_message);
  }

  std::string index_payload;
  err = pending.ReadBlock(index_handle, index_payload);
  if (err) {
    return err;
  }
  pending.index_ = IndexBlock::Decode(index_payload);
  if (!pending.index_) {
    return std::make_error_code(std::errc::bad_message);
  }

  const auto data_end = index_handle.offset;
  std::uint64_t expected_offset = 0;
  for (std::size_t i = 0; i < pending.index_->Size(); ++i) {
    const auto& handle = pending.index_->At(i).handle;
    if (handle.offset != expected_offset || handle.offset > data_end) {
      return std::make_error_code(std::errc::bad_message);
    }
    const auto remaining = data_end - handle.offset;
    if (remaining < 4 || handle.size > remaining - 4) {
      return std::make_error_code(std::errc::bad_message);
    }
    expected_offset = handle.offset + handle.size + 4;
  }
  if (expected_offset != data_end) {
    return std::make_error_code(std::errc::bad_message);
  }

  fd_ = std::exchange(pending.fd_, -1);
  index_ = std::move(pending.index_);
  return {};
}

std::error_code SSTableReader::Close() noexcept {
  index_.reset();
  if (fd_ == -1) {
    return {};
  }

  const int fd = std::exchange(fd_, -1);
  if (::close(fd) == -1) {
    return std::error_code(errno, std::generic_category());
  }
  return {};
}

std::error_code SSTableReader::ReadExactly(std::uint64_t offset, std::span<char> output) {
  std::size_t read_size = 0;
  while (read_size < output.size()) {
    const auto ret = ::pread(fd_, output.data() + read_size,
                             output.size() - read_size,
                             static_cast<off_t>(offset + read_size));
    if (ret > 0) {
      read_size += static_cast<std::size_t>(ret);
    } else if (ret == 0) {
      return std::make_error_code(std::errc::bad_message);
    } else if (errno != EINTR) {
      return std::error_code(errno, std::generic_category());
    }
  }
  return {};
}

std::error_code SSTableReader::ReadBlock(const BlockHandle& handle, std::string& payload) {
  const auto size = static_cast<std::size_t>(handle.size);
  std::string block(size + 4, '\0');
  const auto err = ReadExactly(handle.offset, std::span<char>(block.data(), block.size()));
  if (err) {
    return err;
  }

  const std::string_view body(block.data(), size);
  std::string_view trailer(block.data() + size, 4);
  std::uint32_t expected_crc;
  std::ignore = GetFixed32(trailer, expected_crc);
  if (Crc32c(body) != expected_crc) {
    return std::make_error_code(std::errc::bad_message);
  }

  block.resize(size);
  payload = std::move(block);
  return {};
}

}
