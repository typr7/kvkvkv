#pragma once

#include <string>

#include "skip_list.h"
#include "lookup_result.h"


namespace kv {

class MemTable {
public:
  bool Add(std::uint64_t sequence, ValueType type, std::string key, std::string value);

  [[nodiscard]]
  LookupResult Get(const std::string& key, std::uint64_t snapshot_sequence) const;

  [[nodiscard]]
  SkipList::Iterator NewIterator() const noexcept {
    return table_.NewIterator();
  }

private:
  SkipList table_;
};

}