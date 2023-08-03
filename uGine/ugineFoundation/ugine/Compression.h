#pragma once

#include <memory>
#include <vector>

namespace ugine {

class CompressorImpl;

class Compressor {
public:
    explicit Compressor(bool inflate);

    bool IsBad() const;
    bool Process(const void* data, u32 dataSize, std::vector<u8>& output);

private:
    std::unique_ptr<CompressorImpl> impl_;
};

} // namespace ugine