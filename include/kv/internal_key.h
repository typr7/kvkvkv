#pragma once

#include <string>
#include <cstdint>


namespace kv {

enum class ValueType: std::uint8_t {
  kDeletion = 0,
  kValue = 1
};

struct InternalKey {
  std::string user_key;
  std::uint64_t sequence;
  ValueType type;
};

// 使用 Functor 比较 InternalKey 的原因：
// 1. 与 STL 的接口自然配置
// 2. 将数据与排序规则分开，定义不同的排序规则
// 3. 可以携带状态，支持用户的自定义 Key 比较器
struct InternalKeyComparator {
  bool operator()(const InternalKey& lhs, const InternalKey& rhs) const;
};

}