#include "kv/data_block.h"
#include "kv/coding.h"


namespace kv {

namespace {

constexpr std::size_t kRecordMetadataSize = 17; // 8 + 1 + 4 + 4

}

bool DataBlockBuilder::Add(const InternalKey& key, std::string_view value) {
  constexpr std::size_t offset_part = sizeof(std::uint32_t);
  const std::size_t record_part = kRecordMetadataSize + key.user_key.size() + value.size();

  const std::size_t preview_size = BlockSize() + record_part + offset_part;
  if (preview_size > kMaxDataBlockSize) {
    return false;
  }

  offsets_.push_back(static_cast<std::uint32_t>(data_.size()));

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


}