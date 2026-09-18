#include <utility>

#include "kv/memtable.h"


namespace kv {

bool MemTable::Add(std::uint64_t sequence, ValueType type, std::string key, std::string value) {
  if (type == ValueType::kDeletion) {
    value.clear();
  }

  InternalKey internal_key = InternalKey{std::move(key), sequence, type};
  
  return table_.Insert(std::move(internal_key), std::move(value));
}

LookupResult MemTable::Get(const std::string& key, std::uint64_t snapshot_sequence) const {
  const SkipList::Node* node = 
    table_.LowerBound(InternalKey{key, snapshot_sequence, ValueType::kValue});
  
  if (node == nullptr || node->key.user_key != key) {
    return LookupResult{LookupState::kNotFound, {}};
  }

  if (node->key.type == ValueType::kDeletion) {
    return LookupResult{LookupState::kDeleted, {}};
  }

  return LookupResult{LookupState::kValue, node->value};
}

}