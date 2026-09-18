#include <utility>

#include "kv/memory_kv.h"


namespace kv
{

void MemoryKV::Put(std::string key, std::string value) {
  // 与 data_[key] = std::move(value) 的不同：operator[] 在 key 不存在时会先默认构造一个 value，再赋值
  data_.insert_or_assign(std::move(key), std::move(value));
}

std::optional<std::string> MemoryKV::Get(const std::string& key) const {
  const auto iter = data_.find(key);

  if (iter == data_.end()) {
    return std::nullopt;
  }

  return iter->second;
}

bool MemoryKV::Delete(const std::string& key) {
  return (data_.erase(key) != 0);
}

}