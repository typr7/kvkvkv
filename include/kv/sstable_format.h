#pragma once

#include <cstdint>
#include <string_view>
#include <cstddef>
#include <string>


namespace kv {

// SSTable layout
// [data block 0][crc32c:u32]
// [data block 1][crc32c:u32]
// ...
// [index block][crc32c:u32]
// [footer:32 bytes]
//
// Footer layout
// [index offset: u64]
// [index size: u64]
// [format_version: u32]
// [magic: 8 bytes]
// [footer crc: 4 bytes]

inline constexpr std::size_t kSSTableFooterSize = 32;
inline constexpr std::uint32_t kSSTableFormatVersion = 1;
inline constexpr std::string_view kSSTableMagic = "KVSTBL01";

struct BlockHandle {
  std::uint64_t offset = 0;
  std::uint64_t size = 0;
};

[[nodiscard]]
std::string EncodeFooter(const BlockHandle& index_handle);

[[nodiscard]]
bool DecodeFooter(std::string_view input, BlockHandle& index_handle);

}