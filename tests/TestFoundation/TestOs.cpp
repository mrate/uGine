#include <gtest/gtest.h>

#include <ugine/Os.h>

#include <Windows.h>

TEST(Os, RunProcess) {
    char args[1024] = "\"c:\\Program Files\\Microsoft VS Code\\Code.exe\" -g \"d:\\osobni\\projects\\graphics\\ugine2\\shaders\\standard.frag.hlsl:15\"";

    STARTUPINFOA si{};
    PROCESS_INFORMATION pi{};

    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    const auto res{ CreateProcessA(nullptr, args, nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi) };
    if (res == 0) {
        std::cout << "Chyba: " << GetLastError() << "\n";
    }
}
