#pragma once

#include <uGine/Vector.h>
#include <ugine/Span.h>
#include <ugine/Ugine.h>

namespace ugine {

class IStream {
public:
    virtual ~IStream() = default;

    virtual size_t Read(u8* dst, size_t size) = 0;
};

class OStream {
public:
    virtual ~OStream() = default;

    virtual size_t Write(Span<const u8> data) = 0;
};

class IMemoryStream final : public IStream {
public:
    explicit IMemoryStream(Span<u8> data)
        : data_{ data } {}

    size_t Read(u8* dst, size_t size) override {
        const auto toRead{ std::min(data_.Size() - cursor_, size) };
        if (toRead) {
            memcpy(dst, &data_[cursor_], toRead);
            cursor_ += toRead;
        }
        return toRead;
    }

private:
    Span<const u8> data_;
    size_t cursor_{};
};

class OMemoryStream final : public OStream {
public:
    explicit OMemoryStream(Vector<u8>& output, size_t limitSize = 0)
        : output_{ output }
        , limit_{ limitSize } {}

    size_t Write(Span<const u8> data) override {
        const auto toWrite{ limit_ == 0 ? data.Size() : std::min(limit_ - output_.Size(), data.Size()) };
        if (toWrite) {
            const auto position{ output_.Size() };
            output_.Resize(output_.Size() + toWrite);
            memcpy(&output_[position], data.Data(), toWrite);
        }
        return toWrite;
    }

private:
    Vector<u8>& output_;
    size_t limit_{};
};

} // namespace ugine