#pragma once

#include <ugine/String.h>

#include <chrono>
#include <string_view>

namespace ugine {

// TODO:
std::wstring ToWString(std::string_view str);
std::string ToString(std::wstring_view str);
std::string ToLower(std::string_view v);
std::string ToUpper(std::string_view v);

std::string FormatTime(long long ms);
template <typename T> std::string FormatTime(T t) {
    return FormatTime(std::chrono::duration_cast<std::chrono::milliseconds>(t).count());
}

} // namespace ugine

template <> struct std::formatter<ugine::String> : std::formatter<const char*> {
    auto format(const ugine::String& s, format_context& ctx) { return std::formatter<const char*>::format(s.Data(), ctx); }
};

template <> struct std::formatter<ugine::StringView> : std::formatter<const char*> {
    auto format(const ugine::StringView& s, format_context& ctx) { return std::formatter<const char*>::format(s.Data(), ctx); }
};
