#include <cerrno>
#include <string_view>
#include <tuple>
#include <utility>
#include <fcntl.h>
#include <system_error>
#include <unistd.h>

#include "kv/wal_writer.h"
#include "kv/wal_frame.h"


namespace kv {

WalWriter::~WalWriter() {
  std::ignore = Close();
}

std::error_code WalWriter::OpenNew(const std::string& path) {
  if (fd_ != -1) {
    return std::make_error_code(std::errc::device_or_resource_busy);
  }

  do {
    int ret = ::open(path.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_APPEND | O_CLOEXEC, 0644);
    if (ret != -1) {
      error_.clear();
      fd_ = ret;
    } else {
      const auto err = errno;
      error_ = std::error_code(err, std::generic_category());
    }
  } while (error_ && error_.value() == EINTR);

  return error_;
}

std::error_code WalWriter::Append(const WalRecord& record) {
  if (fd_ == -1) {
    return std::make_error_code(std::errc::bad_file_descriptor);
  }

  if (error_) {
    return error_;
  }

  std::string wal_frame;
  if (!EncodeWalFrame(record, wal_frame)) {
    return std::make_error_code(std::errc::invalid_argument);
  }

  std::string_view remain = wal_frame;
  while (!remain.empty()) {
    const auto writed = ::write(fd_, remain.data(), remain.size());

    if (writed > 0) {
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

std::error_code WalWriter::Sync() {
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

std::error_code WalWriter::Close() {
  if (fd_ == -1) {
    return {};
  }

  int fd = std::exchange(fd_, -1);
  // Linux 上 close 报错时，文件描述符也可能已被释放；不能盲目重试，
  // 否则可能关闭同一进程中其他线程或代码复用该编号后打开的文件。
  if (::close(fd) == -1) {
    const auto err = errno;
    error_ = std::error_code(err, std::generic_category());
    return error_;
  }

  return {};
}

}