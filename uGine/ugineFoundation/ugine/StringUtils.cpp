#include "StringUtils.h"
#include "String.h"

#include <codecvt>
#include <locale>

#include <algorithm>
#include <iostream>
#include <regex>
#include <sstream>

#pragma warning(push)
#pragma warning(disable : 4996)

namespace ugine {

std::wstring ToWString(std::string_view str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str.data());
}

std::string ToString(std::wstring_view str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.to_bytes(str.data());
}

std::string ToLower(std::string_view v) {
    std::string lower{ v };
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });
    return lower;
}

std::string ToUpper(std::string_view v) {
    std::string lower{ v };
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::toupper(c); });
    return lower;
}

//
//void DumpMatrix(const glm::mat4& mat) {
//    for (int i = 0; i < 4; ++i) {
//        for (int j = 0; j < 4; ++j) {
//            std::cout << mat[i][j] << ", ";
//        }
//    }
//}
//
//std::string CamelCaseToHuman(std::string_view camel) {
//    std::stringstream str;
//
//    bool first{ true };
//    for (char c : camel) {
//        if (first) {
//            str << static_cast<char>(std::toupper(c));
//            first = false;
//        } else if (std::isupper(c)) {
//            str << ' ' << static_cast<char>(std::tolower(c));
//        } else {
//            str << c;
//        }
//    }
//
//    return str.str();
//}
//
//bool Contains(std::string_view v, std::string_view what, bool ignoreCase) {
//    if (ignoreCase) {
//        return v.find(what) != std::string::npos;
//    } else {
//        std::string lv{ ToLower(v) };
//        std::string lwhat{ ToLower(what) };
//        return lv.find(lwhat) != std::string::npos;
//    }
//}
//
std::string FormatTime(long long millis) {
    std::stringstream output;

    long ms{ millis % 1000 };
    int seconds{ static_cast<int>(millis / 1000) };
    int minutes{ seconds / 60 };
    int hours{ minutes / 60 };
    int days{ hours / 24 };

    if (days > 0) {
        output << days << "d";
        hours %= 24;
    }

    if (days > 0 || hours > 0) {
        output << hours << "h";
        minutes %= 60;
    }

    if (days > 0 || hours > 0 || minutes > 0) {
        output << minutes << "m";
        seconds %= 60;
    }

    output << seconds << "." << ms << "s";

    return output.str();
}
//
//void Replace(std::string& src, const std::map<std::string, std::string>& values) {
//    std::stringstream filter;
//    bool first{ true };
//    for (auto [name, value] : values) {
//        if (!first) {
//            filter << "|";
//        }
//        filter << name;
//        first = false;
//    }
//
//    std::regex reInclude{ filter.str() };
//
//    std::sregex_iterator begin(src.begin(), src.end(), reInclude);
//    std::sregex_iterator end;
//
//    u64 pos{};
//    std::stringstream out;
//    for (auto i = begin; i != end; ++i) {
//        std::smatch matches{ *i };
//
//        auto it{ values.find(matches.str()) };
//        UGINE_ASSERT(it != values.end());
//
//        out << src.substr(pos, matches.position() - pos) << it->second;
//        pos = matches.position() + matches.length();
//    }
//
//    if (pos > 0) {
//        out << src.substr(pos, src.size() - pos);
//        src = out.str();
//    }
//}
} // namespace ugine

#pragma warning(pop)
