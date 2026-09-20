#include <utility>

#include "kv/wal_recovery.h"
#include "kv/wal_reader.h"
#include "kv/wal_record.h"


namespace kv {

RecoveryResult RecoverWal(const std::string& path) {
  WalReader wal_reader;

  {
    const std::error_code err = wal_reader.Open(path);
    if (err) {
      return {RecoveryStatus::kIoError, err, 0, 0, nullptr};
    }
  }

  RecoveryResult recovery_result;
  auto mem_table = std::make_unique<MemTable>();
  WalReadResult read_result;

  do {
    WalRecord record;

    read_result = wal_reader.Next(record);

    switch (read_result.status) {
      case WalReadStatus::kRecord:
        if (record.sequence > recovery_result.last_sequence) {
          recovery_result.last_sequence = record.sequence;
          recovery_result.valid_bytes = wal_reader.valid_bytes();
          // 由于 record.sequence > recovery_result.last_sequence，Add 不会返回 false
          if (mem_table->Add(record.sequence,
                             record.type,
                             std::move(record.key),
                             std::move(record.value))) [[unlikely]] {
            recovery_result.status = RecoveryStatus::kCorruption;
            read_result.status = WalReadStatus::kCorruption;
          }
        } else {
          recovery_result.status = RecoveryStatus::kCorruption;
          read_result.status = WalReadStatus::kCorruption;
        }
        break;

      case WalReadStatus::kEof: {
        const auto err = wal_reader.Close();
        if (err) {
          recovery_result.status = RecoveryStatus::kIoError;
          recovery_result.error = err;
        } else {
          recovery_result.status = RecoveryStatus::kOk;
          recovery_result.table = std::move(mem_table);
        }
        break;
      }

      case WalReadStatus::kTruncated:
        recovery_result.status = RecoveryStatus::kTruncated;
        break;

      case WalReadStatus::kCorruption:
        recovery_result.status = RecoveryStatus::kCorruption;
        break;

      case WalReadStatus::kIoError:
        recovery_result.status = RecoveryStatus::kIoError;
        recovery_result.error = read_result.error;
        break;
    }
  } while (read_result.status == WalReadStatus::kRecord);

  return recovery_result;
}

}