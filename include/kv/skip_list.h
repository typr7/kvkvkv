#pragma once

#include <array>
#include <vector>
#include <memory>
#include <random>
#include <string>

#include "internal_key.h"


namespace kv {

class SkipList {
public:
  static constexpr int kMaxHeight = 16;

  struct Node {
    InternalKey key;
    std::string value;
    std::array<Node*, kMaxHeight> next{};
  };

  SkipList() = default;

  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;
  SkipList(SkipList&&) = delete;
  SkipList& operator=(SkipList&&) = delete;

  bool Insert(InternalKey key, std::string value);

  [[nodiscard]]
  const Node* LowerBound(const InternalKey& target) const;

private:
  int RandomHeight();

  Node head_;
  int height_ = 1;
  InternalKeyComparator less_;
  std::mt19937 rng_{42};
  std::vector<std::unique_ptr<Node>> nodes_;
};

}