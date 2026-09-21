#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "internal_key.h"


namespace kv {

inline constexpr std::size_t kMaxDataBlockSize = 16 * 1024 * 1024; // 16MiB

// data block layout:
// [record 0][record 1][record 2] ... [record N - 1]
// [offset 0: uint32_t][offset 1][offset 2] ... [offset N - 1]
// [num_records N: uint32_t]
//
// record layout:
// [sequence: uint64_t]
// [type: 1 byte]
// [key_length: uint32_t]
// [value_length: uint32_t]
// [key: key_length bytes]
// [value: value_length bytes]
class DataBlockBuilder {
public:
  [[nodiscard]]
  bool Add(const InternalKey& key, std::string_view value);

  [[nodiscard]]
  std::string Finish() const;

  void Reset();

  [[nodiscard]]
  bool Empty() const noexcept {
    return offsets_.empty();
  }

  [[nodiscard]]
  std::size_t BlockSize() const noexcept {
    return data_.size() + offsets_.size() * sizeof(std::uint32_t) + sizeof(std::uint32_t);
  }

private:
  std::string data_;
  std::vector<std::uint32_t> offsets_;
};

}