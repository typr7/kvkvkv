#pragma once

#include <cstdint>
#include <system_error>
#include <memory>
#include <span>
#include <string>

#include "sstable_format.h"
#include "index_block.h"


namespace kv {

class SSTableReader {
public:
    SSTableReader() = default;
    ~SSTableReader();

    SSTableReader(const SSTableReader&) = delete;
    SSTableReader& operator=(const SSTableReader&) = delete;
    SSTableReader(SSTableReader&&) = delete;
    SSTableReader& operator=(SSTableReader&&) = delete;

    [[nodiscard]]
    std::error_code Open(const std::string& path);

    [[nodiscard]]
    std::error_code Close() noexcept;

private:
    // 读取区间须已验证处于文件内。
    [[nodiscard]]
    std::error_code ReadExactly(std::uint64_t offset, std::span<char> output);

    // handle 的大小和文件范围须已验证。
    [[nodiscard]]
    std::error_code ReadBlock(const BlockHandle& handle, std::string& payload);

    int fd_ = -1;
    std::unique_ptr<IndexBlock> index_;
};


}