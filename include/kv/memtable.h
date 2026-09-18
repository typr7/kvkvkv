#pragma once

#include <cstdint>
#include <string>

#include "skip_list.h"


namespace kv {

enum class LookupState: uint8_t {
  kNotFound = 0,
  kValue = 1,
  kDeleted = 2
};

struct LookupResult {
  LookupState state = LookupState::kNotFound;
  std::string value;
};

class MemTable {
public:
  bool Add(std::uint64_t sequence, ValueType type, std::string key, std::string value);

  [[nodiscard]]
  LookupResult Get(const std::string& key, std::uint64_t snapshot_sequence) const;

private:
  SkipList table_;
};

}