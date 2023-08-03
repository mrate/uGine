#ifdef WIN32

#include "StackTrace.h"
#include "Log.h"

#include <Windows.h>

#include <DbgHelp.h>

#include <array>

namespace ugine {

struct StackTraceInitialize {
    StackTraceInitialize() {
        DWORD error{};

        // Cannot use GetCurrentProcess() when running inside debugger.
        const auto procId{ GetCurrentProcessId() };
        hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, procId);
        //auto hProcess{ GetCurrentProcess() };

        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_DEFERRED_LOADS);

        if (!SymInitialize(hProcess, NULL, TRUE)) {
            // SymInitialize failed
            error = GetLastError();
            UGINE_DEBUG("SymInitialize returned error: {}", error);
        } else {
            Initialized = true;
        }
    }

    ~StackTraceInitialize() { CloseHandle(hProcess); }

    HANDLE hProcess{};
    bool Initialized{};
};

bool Initialize() {
    static StackTraceInitialize init{};
    return init.Initialized;
}

StackTrace::StackTrace(IAllocator& allocator)
    : allocator_{ allocator } {
}

void StackTrace::Capture() {
    std::array<void*, 256> backtrace;
    // Skip first frame (which will be StackTrace::Capture).
    const auto count{ CaptureStackBackTrace(1, 256, backtrace.data(), nullptr) };

    frames_.Reserve(count);

    for (u32 i{}; i < count; ++i) {
        const auto [source, line] = GetAddressSource(backtrace[i]);

        frames_.EmplaceBack(StackFrame{
            .filename = source,
            .symbol = GetSymbolName(backtrace[i]),
            .line = line,
            .address = (u64)backtrace[i],
        });
    }
}

String StackTrace::GetSymbolName(void* address) {
    auto hProcess{ GetCurrentProcess() };

    DWORD64 dwDisplacement{};
    DWORD64 dwAddress = (DWORD64)address;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = MAX_SYM_NAME;

    String result{ allocator_ };
    if (SymFromAddr(hProcess, dwAddress, &dwDisplacement, pSymbol)) {
        result = pSymbol->Name;
    } else {
        //const DWORD error{ GetLastError() };
        //UGINE_DEBUG("SymFromAddr returned error: {}", error);
    }

    return result;
}

std::pair<String, u32> StackTrace::GetAddressSource(void* address) {
    auto hProcess{ GetCurrentProcess() };

    DWORD dwDisplacement{};
    DWORD64 dwAddress = (DWORD64)address;

    char buffer[sizeof(IMAGEHLP_LINE) + MAX_SYM_NAME * sizeof(TCHAR)];
    PIMAGEHLP_LINE pLine = (PIMAGEHLP_LINE)buffer;

    pLine->SizeOfStruct = sizeof(IMAGEHLP_LINE);

    if (SymGetLineFromAddr(hProcess, dwAddress, &dwDisplacement, pLine)) {
        return std::make_pair(String{ pLine->FileName, allocator_ }, pLine->LineNumber);
    } else {
        //const DWORD error{ GetLastError() };
        //UGINE_DEBUG("SymFromAddr returned error: {}", error);
    }

    return std::make_pair(String{ "", allocator_ }, 0);
}

//

u32 GetSymbolCompressed(void* address, Span<u8> output) {
    if (output.Size() < 4) {
        return 0;
    }

    auto hProcess{ GetCurrentProcess() };

    DWORD64 dwDisplacement{};
    DWORD64 dwAddress = (DWORD64)address;

    char buffer[sizeof(SYMBOL_INFO) + MAX_SYM_NAME * sizeof(TCHAR)];
    PSYMBOL_INFO pSymbol = (PSYMBOL_INFO)buffer;

    pSymbol->SizeOfStruct = sizeof(SYMBOL_INFO);
    pSymbol->MaxNameLen = u32(output.Size() - 1);

    u32 result{};
    if (SymFromAddr(hProcess, dwAddress, &dwDisplacement, pSymbol)) {
        result = std::min<u32>(u32(strlen(pSymbol->Name)), pSymbol->MaxNameLen) + 1;
        memcpy(output.Data(), pSymbol->Name, result - 1);

        UGINE_ASSERT(result <= output.Size());
        output.Data()[result - 1] = '\n';
    } else {
        *output.Data() = '\n';
        result = 1;
    }

    return result;
}

u32 GetSourceFileCompressed(void* address, Span<u8> output) {
    if (output.Size() < 8) {
        return 0;
    }

    auto hProcess{ GetCurrentProcess() };

    DWORD dwDisplacement{};
    DWORD64 dwAddress = (DWORD64)address;

    char buffer[sizeof(IMAGEHLP_LINE) + MAX_SYM_NAME * sizeof(TCHAR)];
    PIMAGEHLP_LINE pLine = (PIMAGEHLP_LINE)buffer;

    pLine->SizeOfStruct = sizeof(IMAGEHLP_LINE);
    auto remaining{ u32(output.Size() - 1) };

    u32 result{};
    if (SymGetLineFromAddr(hProcess, dwAddress, &dwDisplacement, pLine)) {
        auto len{ u32(strlen(pLine->FileName)) };
        u32 start{};
        for (start = u32(len - 1); start > 0; --start) {
            if (pLine->FileName[start] == '\\') {
                ++start;
                break;
            }
        }
        len = len - start;

        result = std::min<u32>(len, remaining) + 1;
        memcpy(output.Data(), pLine->FileName + start, result - 1);
        start = result - 1;

        if (start < remaining - 1) {
            output.Data()[start++] = ':';

            const auto digits{ int(log10(pLine->LineNumber)) + 1 };
            if (start < remaining - digits - 1) {
                auto lineNumber{ pLine->LineNumber };
                for (int digit{}; digit < digits; ++digit) {
                    output.Data()[start + digits - digit - 1] = '0' + (lineNumber % 10);
                    lineNumber /= 10;
                }
                start += digits;
            }
        }

        UGINE_ASSERT(start < output.Size());
        output.Data()[start++] = '\n';
        result = start;
    } else {
        *output.Data() = '\n';
        result = 1;
    }

    return result;
}

u32 CaptureStackTraceCompressed(Span<u8> output, int skipFrames, bool byFileName) {
    if (!Initialize()) {
        memset(output.Data(), 0, output.Size());
        return 0;
    }

    std::array<void*, 256> backtrace;
    const auto numFrames{ CaptureStackBackTrace(skipFrames, 256, backtrace.data(), nullptr) };

    u32 captured{};
    int frame{};
    while (captured < output.Size() && frame < numFrames) {
        Span remaining{ output.Data() + captured, output.Size() - captured - 1 };
        const auto c{ byFileName ? GetSourceFileCompressed(backtrace[frame], remaining) : GetSymbolCompressed(backtrace[frame], remaining) };
        if (c == 0) {
            break;
        }
        ++frame;
        captured += c;
    }

    UGINE_ASSERT(captured < output.Size());
    output.Data()[captured] = '\0';
    return captured;
}

} // namespace ugine

#endif