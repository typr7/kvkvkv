#include "kv/crc32c.h"


namespace kv {

std::uint32_t Crc32c(std::string_view data) noexcept {
  std::uint32_t crc = 0xffffffff;

  for (char ch: data) {
    crc ^= static_cast<std::uint8_t>(ch);

    for (int bit = 0; bit < 8; bit++) {
      if ((crc & 1) != 0) {
        crc = (crc >> 1) ^ 0x82f63b78u;
      } else {
        crc >>= 1;
      }
    }
  }

  return ~crc;
}

}