#include <format>
#include <iostream>
#include <vector>

#include <ugine/StackTrace.h>

using namespace ugine;

void someOtherFunc() {
    std::cout << "Source files:\n\n";

    StackTrace stackTrace;
    stackTrace.Capture();

    std::cout << "Captured stack frames:\n";
    for (const auto& frame : stackTrace.Frames()) {
        std::cout << std::format("[0x{:x}] {} ({}:{})\n", frame.address, frame.symbol.Data(), frame.filename.Data(), frame.line);
    }
}

void func1() {
    someOtherFunc();
}

int main(int argc, char* argv[]) {
    std::cout << "Some file\n";

    func1();

    return 0;
}