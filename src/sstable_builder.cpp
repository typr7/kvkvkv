#include "kv/sstable_builder.h"


namespace kv {

constexpr std::size_t kTargetDataBlockSize = 4 * 1024; // 4KiB

std::error_code SSTableBuilder::OpenNew(const std::string& path) {
  std::error_code err = CheckState(State::kInitial);
  if (err) {
    return err;
  }
  
  err = writer_.OpenNew(path);
  if (err) {
    return err;
  }

  state_ = State::kBuilding;

  return {};
}

std::error_code SSTableBuilder::Add(const InternalKey& key, std::string_view value) {
  std::error_code err = CheckState(State::kBuilding);
  if (err) {
    return err;
  }

  if (!data_block_.Add(key, value)) {
    if (data_block_.Empty()) {
      return RecordFailure(std::make_error_code(std::errc::value_too_large));
    } else {
      const auto err = FlushDataBlock();
      if (err) {
        return RecordFailure(err);
      }
      if (!data_block_.Add(key, value)) {
        return RecordFailure(std::make_error_code(std::errc::value_too_large));
      }
    }
  }

  last_key_ = key;

  if (data_block_.BlockSize() >= kTargetDataBlockSize) {
    const auto err = FlushDataBlock();
    if (err) {
      return RecordFailure(err);
    }
  }

  return {};
}

std::error_code SSTableBuilder::Finish() {
  std::error_code err = CheckState(State::kBuilding);
  if (err) {
    return err;
  }

  err = FlushDataBlock();
  if (err) {
    return RecordFailure(err);
  }

  std::string index_block;

  if (!EncodeIndexBlock(index_entries_, index_block)) {
    return RecordFailure(std::make_error_code(std::errc::value_too_large));
  }

  BlockHandle index_handle;

  err = writer_.AppendBlock(index_block, index_handle);
  if (err) {
    return RecordFailure(err);
  }

  err = writer_.Finish(index_handle);
  if (err) {
    return RecordFailure(err);
  }

  state_ = State::kFinished;

  return {};
}

std::error_code SSTableBuilder::FlushDataBlock() {
  if (data_block_.Empty()) {
    return {};
  }

  std::string data_block = data_block_.Finish();

  BlockHandle handle;

  const auto err = writer_.AppendBlock(data_block, handle);
  if (err) {
    return err;
  }

  index_entries_.emplace_back(std::move(last_key_), handle);

  data_block_.Reset();

  return {};
}

std::error_code SSTableBuilder::CheckState(State target) const noexcept {
  if (state_ != target) {
    if (state_ == State::kFailed) {
      return error_;
    } else {
      return std::make_error_code(std::errc::operation_not_permitted);
    }
  }
  
  return {};
}

std::error_code SSTableBuilder::RecordFailure(std::error_code error) {
  state_ = State::kFailed;
  error_ = error;
  return error;
}

}