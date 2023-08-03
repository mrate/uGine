#pragma once

#include <ugine/Path.h>
#include <ugine/Span.h>
#include <ugine/Ugine.h>
#include <ugine/Vector.h>

#include <fstream>
#include <string_view>
#include <vector>

namespace ugine {

// TODO: Error handling.
std::string ReadFile(const Path& path);
std::vector<std::string> ReadFileLines(const Path& path, bool skipEmpty);
std::vector<std::wstring> ReadFileLinesW(const Path& path, bool skipEmpty);

Vector<u8> ReadFileBinary(const Path& path);
Vector<u8> ReadFileBinary(std::istream& stream);

template <typename T> inline void WriteFileLines(const Path& path, const T& lines) {
    std::ofstream file{ path };
    for (const auto& i : lines) {
        file << i << std::endl;
    }
    file.close();
}

template <typename T> inline void WriteFileLinesW(const Path& path, const T& lines) {
    std::wofstream file{ path };
    for (const auto& i : lines) {
        file << i << std::endl;
    }
    file.close();
}

bool WriteFileBinary(const Path& file, Span<const u8> data);

inline bool WriteFileBinary(const Path& file, const Vector<u8>& data) {
    return WriteFileBinary(file, data.ToSpan());
}

// fstream utils.
//void WriteString(std::ofstream& out, std::string_view str);
//void WriteU16(std::ofstream& out, u16 val);
//void WriteU32(std::ofstream& out, u32 val);
//void WriteU64(std::ofstream& out, u64 val);
//
//std::string ReadString(std::ifstream& in);
//u16 ReadU16(std::ifstream& in);
//u32 ReadU32(std::ifstream& in);
//u64 ReadU64(std::ifstream& in);

} // namespace ugine
