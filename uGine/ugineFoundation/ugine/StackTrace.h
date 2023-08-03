#pragma once

#include <ugine/Memory.h>
#include <ugine/String.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

namespace ugine {

class StackTrace {
public:
    struct StackFrame {
        String filename;
        String symbol;
        u32 line{};
        u64 address{};
    };

    explicit StackTrace(IAllocator& allocator = IAllocator::Default());
    void Capture();
    Span<const StackFrame> Frames() const { return frames_.ToSpan(); }

private:
    String GetSymbolName(void* address);
    std::pair<String, u32> GetAddressSource(void* address);

    AllocatorRef allocator_;
    Vector<StackFrame> frames_;
};

u32 CaptureStackTraceCompressed(Span<u8> output, int skipFrames = 1, bool byFileName = false);

} // namespace ugine
