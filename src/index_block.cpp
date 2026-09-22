#include <tuple>

#include "kv/index_block.h"
#include "kv/data_block.h"
#include "kv/coding.h"


namespace kv {

std::unique_ptr<IndexBlock> IndexBlock::Decode(std::string_view input) {
  if (input.size() < sizeof(std::uint32_t) || input.size() > kMaxIndexBlockSize) {
    return nullptr;
  }

  std::uint32_t num_entries;
  std::ignore = GetFixed32(input, num_entries);

  if (num_entries == 0 && input.size() != 0) {
    return nullptr;
  }

  if (num_entries > input.size() / kMinIndexBlockEntrySize) {
    return nullptr;
  }

  std::unique_ptr<IndexBlock> block(new IndexBlock);
  block->entries_.reserve(num_entries);
  InternalKeyComparator less;

  for (std::uint32_t i = 0; i < num_entries; i++) {
    if (input.size() < kMinIndexBlockEntrySize) {
      return nullptr;
    }

    std::uint32_t key_len;
    std::ignore = GetFixed32(input, key_len);

    if (key_len > input.size() - 25) {
      return nullptr;
    }

    Entry entry;

    // user_key
    entry.key.user_key = std::string(input.substr(0, key_len));
    input.remove_prefix(key_len);

    // sequence
    std::ignore = GetFixed64(input, entry.key.sequence);

    // type
    char type_val = input.front();
    if (type_val != static_cast<char>(ValueType::kValue)
        && type_val != static_cast<char>(ValueType::kDeletion)) {
      return nullptr;
    }
    entry.key.type = static_cast<ValueType>(type_val);
    input.remove_prefix(1);

    if (!block->entries_.empty() && !less(block->entries_.back().key, entry.key)) {
      return nullptr;
    }

    // offset
    std::ignore = GetFixed64(input, entry.handle.offset);

    if (!block->entries_.empty() && block->entries_.back().handle.offset >= entry.handle.offset) {
      return nullptr;
    }
    
    // size
    std::ignore = GetFixed64(input, entry.handle.size);

    if (entry.handle.size < sizeof(std::uint32_t) || entry.handle.size > kMaxDataBlockSize) {
      return nullptr;
    }

    block->entries_.push_back(std::move(entry));
  }

  if (input.size() != 0) {
    return nullptr;
  }

  return block;
}

std::size_t IndexBlock::LowerBound(const InternalKey& target) const {
  InternalKeyComparator less;
  int left = 0;
  int right = static_cast<int>(entries_.size());

  while (left < right) {
    int mid = left + ((right - left) >> 1);
    if (less(entries_[mid].key, target)) {
      left = mid + 1;
    } else {
      right = mid;
    }
  }

  return static_cast<std::size_t>(left);
}

bool EncodeIndexBlock(const std::vector<IndexBlock::Entry> &entries, std::string &output) {
  std::string block;

  std::size_t block_size = sizeof(std::uint32_t);

  for (const auto& entry: entries) {
    block_size += kMinIndexBlockEntrySize + entry.key.user_key.size();
    if (block_size > kMaxIndexBlockSize) {
      return false;
    }
  }

  block.reserve(block_size);
  
  // N
  PutFixed32(block, static_cast<std::uint32_t>(entries.size()));

  for (const auto& entry: entries) {
    // key_length
    PutFixed32(block, static_cast<std::uint32_t>(entry.key.user_key.size()));
    // user_key
    block.append(entry.key.user_key);
    // sequence
    PutFixed64(block, entry.key.sequence);
    // type
    block.push_back(static_cast<char>(entry.key.type));
    // offset
    PutFixed64(block, entry.handle.offset);
    // size
    PutFixed64(block, entry.handle.size);
  }

  output = std::move(block);

  return true;
}

}