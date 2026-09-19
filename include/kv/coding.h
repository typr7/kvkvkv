#pragma once

#include <cstdint>
#include <string>
#include <string_view>


namespace kv {

void PutFixed32(std::string& dst, std::uint32_t value);
void PutFixed64(std::string& dst, std::uint64_t value);

[[nodiscard]]
bool GetFixed32(std::string_view& input, std::uint32_t& value) noexcept;

[[nodiscard]]
bool GetFixed64(std::string_view& input, std::uint64_t& value) noexcept;

}