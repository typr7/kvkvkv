#pragma once

#include <memory>
#include <vector>

#include "internal_key.h"
#include "sstable_format.h"


namespace kv {

inline constexpr std::size_t kMaxIndexBlockSize = 16 * 1024 * 1024; // 16MiB

inline constexpr std::size_t kMinIndexBlockEntrySize = 29;

// IndexBlock layout
// [N:u32][entry 0][entry 1] ... [entry N - 1]
//
// Entry layout
// [key_length:u32]
// [user_key]
// [sequence:u64]
// [type:u8]
// [offset:u64]
// [size:u64]
class IndexBlock {
public:
  struct Entry {
    InternalKey key;
    BlockHandle handle;
  };

  [[nodiscard]]
  static std::unique_ptr<IndexBlock> Decode(std::string_view input);

  [[nodiscard]]
  std::size_t Size() const noexcept {
    return entries_.size();
  }

  [[nodiscard]]
  const Entry& At(std::size_t index) const {
    return entries_[index];
  }

  [[nodiscard]]
  std::size_t LowerBound(const InternalKey& target) const;

private:
  IndexBlock() = default;

  std::vector<Entry> entries_;
  InternalKeyComparator less_;
};

[[nodiscard]]
bool EncodeIndexBlock(const std::vector<IndexBlock::Entry>& entries, std::string& output);

}