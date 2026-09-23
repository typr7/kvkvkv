#pragma once

#include <string>
#include <system_error>
#include <vector>

#include "internal_key.h"
#include "sstable_writer.h"
#include "data_block.h"
#include "index_block.h"


namespace kv {

class SSTableBuilder {
public:
    [[nodiscard]]
    std::error_code OpenNew(const std::string& path);

    [[nodiscard]]
    std::error_code Add(const InternalKey& key, std::string_view value);

    [[nodiscard]]
    std::error_code Finish();

private:
    enum class State {
        kInitial,
        kBuilding,
        kFinished,
        kFailed,
    };

    [[nodiscard]]
    std::error_code FlushDataBlock();

    [[nodiscard]]
    std::error_code CheckState(State target) const noexcept;

    [[nodiscard]]
    std::error_code RecordFailure(std::error_code error);

    SSTableWriter writer_;
    DataBlockBuilder data_block_;
    std::vector<IndexBlock::Entry> index_entries_;
    InternalKey last_key_;

    State state_ = State::kInitial;
    std::error_code error_;
};

}