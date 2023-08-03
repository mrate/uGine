#pragma once

#include <ugine/Log.h>
#include <ugine/Ugine.h>

#include <format>
#include <stdexcept>
#include <string>

namespace ugine {

class Error : public std::exception {
public:
    Error(const char* msg, const char* name, const char* file, int line, const char* function) noexcept
        : std::exception{ msg }
        , name_{ name }
        , file_{ file }
        , line_{ line }
        , function_{ function } {

        UGINE_WARN("Exception thrown at {}:{} {}()", file_, line_, function_);
        //UGINE_BREAK;
    }

    const char* Name() const noexcept { return name_; }
    const char* File() const noexcept { return file_; }
    int Line() const noexcept { return line_; }
    const char* Function() const noexcept { return function_; }
    std::string ToString() const { return std::format("{}: {} [thrown at {}:{} {}()]", name_, what(), file_, line_, function_); }

private:
    const char* name_{};
    const char* file_{};
    const int line_{};
    const char* function_{};
};

} // namespace ugine

#define UGINE_THROW(T, MSG, ...) throw T(MSG, #T, __FILE__, __LINE__, __func__, __VA_ARGS__)
