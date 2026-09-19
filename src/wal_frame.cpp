#include <utility>
#include <tuple>

#include "kv/wal_frame.h"
#include "kv/coding.h"
#include "kv/crc32c.h"


namespace kv {

bool EncodeWalFrame(const WalRecord& record, std::string& output) {
  // payload
  std::string payload;
  if(!EncodeWalPayload(record, payload)) {
    return false;
  }

  std::string wal_frame;

  // payload length
  PutFixed32(wal_frame, static_cast<std::uint32_t>(payload.size()));
  // payload crc
  PutFixed32(wal_frame, Crc32c(payload));
  // header crc
  PutFixed32(wal_frame, Crc32c(wal_frame));
  // payload
  wal_frame.append(payload);

  output = std::move(wal_frame);

  return true;
}

WalDecodeStatus DecodeWalFrame(std::string_view& input, WalRecord& output) {
  auto in = input;

  // read header
  if (in.size() < kWalFrameHeaderSize) {
    return WalDecodeStatus::kIncomplete;
  }

  std::uint32_t payload_len;
  std::uint32_t payload_crc;
  std::uint32_t header_crc;
  std::ignore = GetFixed32(in, payload_len);
  std::ignore = GetFixed32(in, payload_crc);
  std::ignore = GetFixed32(in, header_crc);

  // header crc check
  if (header_crc != Crc32c(input.substr(0, 8))) {
    return WalDecodeStatus::kCorruption;
  }

  // check payload
  if (payload_len < kWalPayloadHeaderSize || payload_len > kMaxWalPayloadSize) {
    return WalDecodeStatus::kCorruption;
  }

  if (in.size() < payload_len) {
    return WalDecodeStatus::kIncomplete;
  }

  // payload crc check
  if (payload_crc != Crc32c(in.substr(0, payload_len))) {
    return WalDecodeStatus::kCorruption;
  }

  // read payload
  WalRecord record;
  if (!DecodeWalPayload(in.substr(0, payload_len), record)) {
    return WalDecodeStatus::kCorruption;
  }
  in.remove_prefix(payload_len);

  input = in;
  output = std::move(record);

  return WalDecodeStatus::kOk;
}

}