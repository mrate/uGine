#include "Os.h"

#include <ugine/Vector.h>

#include <Windows.h>

#include <sstream>

namespace ugine {

bool RunProcess(StringView process, Span<String> arguments) {
    Vector<char> args;
    args.Reserve(32 * 1024);

    args.Append(process.Data(), process.Size());
    args.PushBack(' ');
    for (const auto& arg : arguments) {
        args.PushBack('\"');
        args.Append(arg.Data(), arg.Size());
        args.PushBack('\"');
        args.PushBack(' ');
    }
    args.PushBack('\0');

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    const auto res{ CreateProcessA(nullptr, args.Data(), nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) };
    if (res == 0) {
        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return true;
}

} // namespace ugine