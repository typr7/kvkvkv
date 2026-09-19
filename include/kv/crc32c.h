#pragma once

#include <cstdint>
#include <string_view>


namespace kv {

[[nodiscard]]
std::uint32_t Crc32c(std::string_view data) noexcept;

}