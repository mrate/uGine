#include "File.h"
#include "Error.h"

#include <fstream>
#include <sstream>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace ugine {

std::string ReadFile(const Path& path) {
    std::string str;

    std::ifstream file{ path.Data() };
    if (file.good()) {
        std::stringstream sourceBuffer;
        sourceBuffer << file.rdbuf();
        str = sourceBuffer.str();
    }

    return str;
}

template <typename T, typename S> std::vector<T> ReadFileLinesT(S& stream, bool skipEmpty) {
    std::vector<T> result;

    if (stream.good()) {
        while (!stream.eof()) {
            T line;
            std::getline(stream, line);
            if (!line.empty() || !skipEmpty) {
                result.push_back(line);
            }
        }
    }

    return result;
}

std::vector<std::string> ReadFileLines(const Path& path, bool skipEmpty) {
    std::ifstream file(path.Data());

    return ReadFileLinesT<std::string>(file, skipEmpty);
}

std::vector<std::wstring> ReadFileLinesW(const Path& path, bool skipEmpty) {
    std::wifstream file(path.Data());

    return ReadFileLinesT<std::wstring>(file, skipEmpty);
}

Vector<u8> ReadFileBinary(const Path& path) {
    std::ifstream file{ path.Data(), std::ios::binary };
    return ReadFileBinary(file);
}

Vector<u8> ReadFileBinary(std::istream& file) {
    if (file.good()) {
        file.seekg(0, std::ios::end);
        const auto size{ file.tellg() };
        file.seekg(0, std::ios::beg);

        Vector<u8> data(size);
        file.read(reinterpret_cast<char*>(data.Data()), size);

        return data;
    }

    return {};
}

bool WriteFileBinary(const Path& path, Span<const u8> data) {
    std::ofstream file{ path.Data(), std::ios::binary };
    if (file.fail()) {
        return false;
    }
    file.write(reinterpret_cast<const char*>(data.Data()), data.Size());
    const auto written{ file.tellp() };
    file.close();

    return written == data.Size();
}

//Path MakeRelative(const Path& srcPath, const Path& file) {
//    if (file.is_absolute()) {
//        return file;
//    } else {
//        auto workingPath{ std::filesystem::current_path() };
//        auto fullPath{ srcPath / file };
//        return std::filesystem::relative(fullPath, workingPath);
//    }
//}
//
//void WriteString(std::ofstream& out, std::string_view str) {
//    out << str;
//
//    WriteU32(out, str.size());
//    out.write(str.data(), str.size());
//}
//
//void WriteU16(std::ofstream& out, u16 val) {
//    out.write(reinterpret_cast<const char*>(&val), 2);
//}
//
//void WriteU32(std::ofstream& out, u32 val) {
//    out.write(reinterpret_cast<const char*>(&val), 4);
//}
//
//void WriteU64(std::ofstream& out, u64 val) {
//    out.write(reinterpret_cast<const char*>(&val), 8);
//}
//
//std::string ReadString(std::ifstream& in) {
//    u32 size{ ReadU32(in) };
//    std::string res('\0', size);
//    in.read(res.data(), size);
//}
//
//u16 ReadU16(std::ifstream& in) {
//    u16 val{};
//    in.read(reinterpret_cast<char*>(&val), 2);
//    return val;
//}
//
//u32 ReadU32(std::ifstream& in) {
//    u32 val{};
//    in.read(reinterpret_cast<char*>(&val), 4);
//    return val;
//}
//
//u64 ReadU64(std::ifstream& in) {
//    u64 val{};
//    in.read(reinterpret_cast<char*>(&val), 8);
//    return val;
//}

} // namespace ugine
