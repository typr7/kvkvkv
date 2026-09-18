#pragma once

#include <map>
#include <optional>
#include <string>


namespace kv
{

class MemoryKV {
public:
  void Put(std::string key, std::string value);

  [[nodiscard]]
  std::optional<std::string> Get(const std::string& key) const;

  bool Delete(const std::string& key);

private:
  // 为什么不用 std::unordered_map: 保证有序
  std::map<std::string, std::string> data_;
};

}