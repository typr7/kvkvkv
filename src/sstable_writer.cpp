#include <tuple>
#include <utility>
#include <fcntl.h>
#include <unistd.h>

#include "kv/sstable_writer.h"
#include "kv/crc32c.h"
#include "kv/coding.h"


namespace kv {

SSTableWriter::~SSTableWriter() {
  std::ignore = Close();
}

std::error_code SSTableWriter::OpenNew(const std::string& path) {
  if (fd_ != -1) {
    return std::make_error_code(std::errc::device_or_resource_busy);
  }

  do {
    int ret = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_APPEND | O_CLOEXEC, 0644);
    if (ret != -1) {
      offset_ = 0;
      error_.clear();
      fd_ = ret;
    } else {
      const auto err = errno;
      error_ = std::error_code(err, std::generic_category());
    }
  } while (error_ && error_.value() == EINTR);

  return error_;
}

std::error_code SSTableWriter::AppendBlock(std::string_view payload, BlockHandle& handle) {
  if (fd_ == -1) {
    return std::make_error_code(std::errc::bad_file_descriptor);
  }

  BlockHandle pending{offset_, payload.size()};

  std::uint32_t crc_val = Crc32c(payload);
  std::string crc;
  PutFixed32(crc, crc_val);

  std::error_code err = WriteAll(payload);
  if (err) {
    return err;
  }

  err = WriteAll(crc);
  if (err) {
    return err;
  }

  handle = pending;

  return {};
}

std::error_code SSTableWriter::Finish(const BlockHandle& index_handle) {
  std::string footer = EncodeFooter(index_handle);

  std::error_code err = WriteAll(footer);
  if (err) {
    return err;
  }

  err = Sync();
  if (err) {
    return err;
  }

  err = Close();
  if (err) {
    return err;
  }

  return {};
}

std::error_code SSTableWriter::Close() {
  if (fd_ == -1) {
    return {};
  }

  int fd = std::exchange(fd_, -1);
  if (::close(fd) == -1) {
    const auto err = errno;
    error_ = std::error_code(err, std::generic_category());
    return error_;
  }

  return {};
}

std::error_code SSTableWriter::WriteAll(std::string_view bytes) {
  if (fd_ == -1) {
    return std::make_error_code(std::errc::bad_file_descriptor);
  }

  if (error_) {
    return error_;
  }

  std::string_view remain = bytes;
  while (!remain.empty()) {
    const auto writed = ::write(fd_, remain.data(), remain.size());

    if (writed > 0) {
      offset_ += static_cast<std::uint64_t>(writed);
      remain.remove_prefix(writed);
      continue;
    }

    const int err = (writed == 0 ? EIO : errno);

    if (err == EINTR) {
      continue;
    }

    error_ = std::error_code(err, std::generic_category());
    return error_;
  }

  return {};
}

std::error_code SSTableWriter::Sync() {
  if (fd_ == -1) {
    return std::make_error_code(std::errc::bad_file_descriptor);
  }

  if (error_) {
    return error_;
  }

  do {
    if (::fdatasync(fd_) != -1) {
      error_.clear();
    } else {
      const auto err = errno;
      error_ = std::error_code(err, std::generic_category());
    }
  } while (error_ && error_.value() == EINTR);

  return error_;
}

}