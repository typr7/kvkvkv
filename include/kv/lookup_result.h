#pragma once

#include <string>
#include <cstdint>


namespace kv {

enum class LookupState: std::uint8_t {
  kNotFound = 0,
  kValue = 1,
  kDeleted = 2
};

struct LookupResult {
  LookupState state = LookupState::kNotFound;
  std::string value;
};

}