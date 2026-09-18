#include <string>

#include <gtest/gtest.h>

#include "kv/memory_kv.h"


TEST(MemoryKVTest, MissingKeyReturnsNullopt) {
  kv::MemoryKV db;

  EXPECT_FALSE(db.Get("missing").has_value());
}

TEST(MemoryKVTest, PutThenGetReturnsValue) {
  kv::MemoryKV db;

  db.Put("name", "alice");
  
  const auto value = db.Get("name");

  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "alice");
}

TEST(MemoryKVTest, PutOverwriteExistingValue) {
  kv::MemoryKV db;

  db.Put("name", "alice");
  db.Put("name", "bob");

  const auto value = db.Get("name");

  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "bob");
}

TEST(MemoryKVTest, DeleteRemovesExistingKey) {
  kv::MemoryKV db;

  db.Put("name", "alice");

  EXPECT_TRUE(db.Delete("name"));
  EXPECT_FALSE(db.Get("name").has_value());
  EXPECT_FALSE(db.Delete("name"));
}

TEST(MemoryKVTest, EmptyKeyAndValueAreValid) {
  kv::MemoryKV db;

  db.Put("", "");

  const auto value = db.Get("");
  
  ASSERT_TRUE(value.has_value());
  EXPECT_TRUE(value->empty());
}

TEST(MemoryKVTest, KeysAndValuesCanContainZeroBytes) {
  kv::MemoryKV db;

  // std::string{"a\0b"} 通常构造为 "a"，这种构造方式以 '\0' 为字符串结尾
  const std::string key("a\0b", 3);
  const std::string expected("x\0y", 3);

  db.Put(key, expected);

  const auto value = db.Get(key);

  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, expected);
  EXPECT_FALSE(db.Get("a").has_value());
}

// appended tests
TEST(MemoryKVTest, RemoveNotExistingKey) {
  kv::MemoryKV db;

  db.Put("name", "alice");

  const auto value = db.Get("name");

  ASSERT_FALSE(db.Delete("age"));
  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "alice");
  ASSERT_TRUE(db.Delete("name"));
}

TEST(MemoryKVTest, PutTwoKeysAndRemoveOne) {
  kv::MemoryKV db;

  db.Put("name", "alice");
  db.Put("age", "13");

  ASSERT_TRUE(db.Delete("name"));
  ASSERT_FALSE(db.Get("name").has_value());

  const auto value = db.Get("age");

  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "13");
}

TEST(MemoryKVTest, PutKeyDeleteItAndPutAgain) {
  kv::MemoryKV db;

  db.Put("name", "alice");

  ASSERT_TRUE(db.Get("name").has_value());
  ASSERT_TRUE(db.Delete("name"));

  db.Put("name", "bob");

  const auto value = db.Get("name");

  ASSERT_TRUE(value.has_value());
  EXPECT_EQ(*value, "bob");
}