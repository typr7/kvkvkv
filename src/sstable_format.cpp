#include <tuple>

#include "kv/sstable_format.h"
#include "kv/coding.h"
#include "kv/crc32c.h"


namespace kv {

std::string EncodeFooter(const BlockHandle& index_handle) {
  std::string footer;
  footer.reserve(kSSTableFooterSize);

  // index offset
  PutFixed64(footer, index_handle.offset);
  // index size
  PutFixed64(footer, index_handle.size);
  // format version
  PutFixed32(footer, kSSTableFormatVersion);
  // magic
  footer.append(kSSTableMagic);
  // crc
  PutFixed32(footer, Crc32c(footer));

  return footer;
}

bool DecodeFooter(std::string_view input, BlockHandle& index_handle) {
  if (input.size() != kSSTableFooterSize) {
    return false;
  }

  std::string_view footer_without_crc = input.substr(0, kSSTableFooterSize - 4);

  std::uint64_t offset;
  std::uint64_t size;
  std::uint32_t version;
  std::uint32_t crc;

  // index offset
  std::ignore = GetFixed64(input, offset);
  // index size
  std::ignore = GetFixed64(input, size);
  // format version
  std::ignore = GetFixed32(input, version);
  if (version != kSSTableFormatVersion) {
    return false;
  }
  // magic
  if (input.substr(0, 8) != kSSTableMagic) {
    return false;
  }
  input.remove_prefix(8);
  // crc
  std::ignore = GetFixed32(input, crc);
  if (crc != Crc32c(footer_without_crc)) {
    return false;
  }

  index_handle = {offset, size};

  return true;
}

}