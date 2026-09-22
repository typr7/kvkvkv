#include <tuple>
#include <utility>

#include "kv/data_block.h"
#include "kv/coding.h"
#include "kv/wal_record.h"


namespace kv {

namespace {

constexpr std::size_t kRecordMetadataSize = kWalPayloadHeaderSize; // 8 + 1 + 4 + 4

}

bool DataBlockBuilder::Add(const InternalKey& key, std::string_view value) {
  constexpr std::size_t offset_part = sizeof(std::uint32_t);
  const std::size_t record_part = kRecordMetadataSize + key.user_key.size() + value.size();

  const std::size_t preview_size = BlockSize() + record_part + offset_part;
  if (preview_size > kMaxDataBlockSize) {
    return false;
  }

  const auto offset = static_cast<std::uint32_t>(data_.size());

  // sequence
  PutFixed64(data_, key.sequence);
  // type
  data_.push_back(static_cast<char>(key.type));
  // key_length
  PutFixed32(data_, static_cast<std::uint32_t>(key.user_key.size()));
  // value_length
  PutFixed32(data_, static_cast<std::uint32_t>(value.size()));
  // key
  data_.append(key.user_key);
  // value
  data_.append(value);

  offsets_.push_back(offset);

  return true;
}

std::string DataBlockBuilder::Finish() const {
  std::string block = data_;

  block.reserve(block.size() + offsets_.size() * sizeof(std::uint32_t) + sizeof(std::uint32_t));

  for (const auto offset: offsets_) {
    PutFixed32(block, offset);
  }

  PutFixed32(block, static_cast<std::uint32_t>(offsets_.size()));

  return block;
}

void DataBlockBuilder::Reset() {
  data_.clear();
  offsets_.clear();
}

// DataBlock

std::unique_ptr<DataBlock> DataBlock::Decode(std::string_view input) {
  if (input.size() < sizeof(std::uint32_t) || input.size() > kMaxDataBlockSize) {
    return nullptr;
  }

  // read num_entries in block tail
  auto num_entries_view = input.substr(input.size() - sizeof(std::uint32_t));
  std::uint32_t num_entries;
  std::ignore = GetFixed32(num_entries_view, num_entries);

  if (num_entries == 0) {
    return (input.size() == sizeof(std::uint32_t)
            ? std::unique_ptr<DataBlock>(new DataBlock)
            : nullptr);
  }

  if (num_entries > (input.size() - sizeof(std::uint32_t))
                     / (kRecordMetadataSize + sizeof(std::uint32_t))) {
    return nullptr;
  }

  const std::size_t entry_chunk_size =
    input.size() - sizeof(std::uint32_t) - num_entries * sizeof(std::uint32_t);
  auto entries_view = input.substr(0, entry_chunk_size);
  auto offsets_view = input.substr(entry_chunk_size, num_entries * sizeof(std::uint32_t));
  
  std::unique_ptr<DataBlock> block(new DataBlock);
  block->entries_.reserve(num_entries);
  InternalKeyComparator less;

  std::uint32_t start;
  std::ignore = GetFixed32(offsets_view, start);
  
  if (start != 0) {
    return nullptr;
  }

  // decode record for 0 to N - 2
  for (std::uint32_t i = 1; i < num_entries; i++) {
    std::uint32_t end;
    std::ignore = GetFixed32(offsets_view, end);

    if (end <= start || end >= entry_chunk_size) {
      return nullptr;
    }

    // DataBlock 的 record 格式与 WAL 的 record 格式一致
    WalRecord record;
    
    if (!DecodeWalPayload(entries_view.substr(start, end - start), record)) {
      return nullptr;
    }

    InternalKey key{std::move(record.key), record.sequence, record.type};

    if (!block->entries_.empty() && !less(block->entries_.back().key, key)) {
      return nullptr;
    }

    block->entries_.emplace_back(std::move(key), std::move(record.value));

    start = end;
  }

  // decode record N - 1
  WalRecord record;

  if (!DecodeWalPayload(entries_view.substr(start), record)) {
    return nullptr;
  }

  InternalKey key{std::move(record.key), record.sequence, record.type};

  if (!block->entries_.empty() && !less(block->entries_.back().key, key)) {
    return nullptr;
  }

  block->entries_.emplace_back(std::move(key), std::move(record.value));

  return block;
}

std::size_t DataBlock::LowerBound(const InternalKey& target) const {

  int left = 0;
  int right = static_cast<int>(entries_.size());

  while (left < right) {
    int mid = left + ((right - left) >> 1);
    if (less_(entries_[mid].key, target)) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  return static_cast<std::size_t>(left);
}

}