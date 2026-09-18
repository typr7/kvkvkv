#include <vector>
#include <algorithm>

#include <gtest/gtest.h>

#include "kv/internal_key.h"


TEST(InternalKeyTest, UserKeyTakesPriorityOverSequence) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey a{.user_key="key_a", .sequence=0, .type=kv::ValueType::kValue};
  const kv::InternalKey b{.user_key="key_b", .sequence=1, .type=kv::ValueType::kValue};

  EXPECT_TRUE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, SequenceTakesPriorityValueType) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey a{.user_key="key", .sequence=1, .type=kv::ValueType::kDeletion};
  const kv::InternalKey b{.user_key="key", .sequence=0, .type=kv::ValueType::kValue};

  EXPECT_TRUE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, SameKey) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey a{.user_key="name", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey b = a;

  EXPECT_FALSE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, ValueTypeComparation) {
  const kv::InternalKeyComparator less;
  
  const kv::InternalKey a{.user_key="name", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey b{.user_key="name", .sequence=1, .type=kv::ValueType::kDeletion};

  EXPECT_TRUE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, EmptyKeyAndNotEmptyKey) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey a{.user_key="", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey b{.user_key="name", .sequence=1, .type=kv::ValueType::kValue};

  EXPECT_TRUE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, ZeroBytesKeyComparation) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey a{.user_key="a", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey b{.user_key=std::string{"a\0b", 3}, .sequence=1, .type=kv::ValueType::kValue};

  EXPECT_TRUE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, SingleByteComparation) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey a{.user_key="\x7f", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey b{.user_key="\x80", .sequence=1, .type=kv::ValueType::kValue};

  EXPECT_TRUE(less(a, b));
  EXPECT_FALSE(less(b, a));
}

TEST(InternalKeyTest, InternalKeySort) {
  const kv::InternalKeyComparator less;

  const kv::InternalKey key0{.user_key="", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey key1{.user_key="key_a", .sequence=0, .type=kv::ValueType::kValue};
  const kv::InternalKey key2{.user_key="key_b", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey key3{.user_key="key", .sequence=1, .type=kv::ValueType::kDeletion};
  const kv::InternalKey key4{.user_key="key", .sequence=0, .type=kv::ValueType::kValue};
  const kv::InternalKey key5{.user_key="key", .sequence=0, .type=kv::ValueType::kDeletion};
  const kv::InternalKey key6{.user_key="a", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey key7{.user_key=std::string{"a\0b", 3}, .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey key8{.user_key="\x7f", .sequence=1, .type=kv::ValueType::kValue};
  const kv::InternalKey key9{.user_key="\x80", .sequence=1, .type=kv::ValueType::kValue};

  std::vector<kv::InternalKey> keys{key0, key1, key2, key3, key4, key5, key6, key7, key8, key9};
  const std::vector<kv::InternalKey> expected{key0, key6, key7, key3, key4, key5, key1, key2, key8, key9};


  const auto check_if_eq = [&lhs = keys, &rhs = expected] {
    if (lhs.size() != rhs.size()) {
      return false;
    }

    for (std::size_t i = 0; i < lhs.size(); i++) {
      const auto& l = lhs[i];
      const auto& r = rhs[i];
      if (l.user_key != r.user_key || l.sequence != r.sequence || l.type != r.type) {
        return false;
      }
    }

    return true;
  };

  EXPECT_FALSE(check_if_eq());
  std::sort(keys.begin(), keys.end(), less);
  EXPECT_TRUE(check_if_eq());
}