#include <utility>
#include <tuple>

#include "kv/wal_record.h"
#include "kv/coding.h"


namespace kv {

bool EncodeWalPayload(const WalRecord& record, std::string& output) {
  if (record.type != ValueType::kValue && record.type != ValueType::kDeletion) {
    return false;
  }

  if (record.type == ValueType::kDeletion && !record.value.empty()) {
    return false;
  }

  constexpr std::size_t budget = kMaxWalPayloadSize - kWalPayloadHeaderSize;

  if (record.key.size() > budget) {
    return false;
  }

  if (record.value.size() > budget - record.key.size()) {
    return false;
  }

  std::string encoded;
  encoded.reserve(kWalPayloadHeaderSize + record.key.size() + record.value.size());

  // sequence
  PutFixed64(encoded, record.sequence);
  // type
  encoded.push_back(static_cast<char>(record.type));
  // key_length
  PutFixed32(encoded, static_cast<std::uint32_t>(record.key.size()));
  // value_length
  PutFixed32(encoded, static_cast<std::uint32_t>(record.value.size()));
  // key
  encoded.append(record.key);
  // value
  encoded.append(record.value);

  output = std::move(encoded);

  return true;
}

bool DecodeWalPayload(std::string_view input, WalRecord& output) {
  if (input.size() < kWalPayloadHeaderSize || input.size() > kMaxWalPayloadSize) {
    return false;
  }

  WalRecord decoded;

  // sequence
  std::ignore = GetFixed64(input, decoded.sequence);

  // type
  char type_val = input.front();
  input.remove_prefix(1);
  if (type_val != static_cast<char>(ValueType::kValue)
      && type_val != static_cast<char>(ValueType::kDeletion)) {
    return false;
  }
  decoded.type = static_cast<ValueType>(type_val);

  // key_length and value_length
  std::uint32_t key_len = 0;
  std::uint32_t value_len = 0;
  std::ignore = GetFixed32(input, key_len);
  std::ignore = GetFixed32(input, value_len);
  
  if (decoded.type == ValueType::kDeletion && value_len != 0) {
    return false;
  }

  if (input.size() != static_cast<std::size_t>(key_len) + value_len) {
    return false;
  }

  // key
  decoded.key = std::string(input.substr(0, key_len));
  input.remove_prefix(key_len);
  // value
  decoded.value = std::string(input);

  output = std::move(decoded);

  return true;
}

}