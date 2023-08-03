#include <algorithm>
#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

using Bytes = std::vector<uint8_t>;
using Dictionary = std::map<std::string, std::string>;

std::string VariableName(const std::string_view& fileName) {
    std::string result{ fileName };

    std::transform(result.begin(), result.end(), result.begin(), [](char c) {
        switch (c) {
        case '.':
        case '-': return '_';
        default: return static_cast<char>(std::toupper(c));
        }
    });
    return result;
}

Bytes ReadContent(const std::filesystem::path& file) {
    std::ifstream in{ file, std::ios::binary };
    if (!in.is_open()) {
        return {};
    }

    in.seekg(0, std::ios::end);
    const auto size{ in.tellg() };
    in.seekg(0, std::ios::beg);
    Bytes content(size, 0);
    in.read(reinterpret_cast<char*>(content.data()), size);

    return content;
}

Bytes ReadContentPreprocess(const std::filesystem::path& file, const Dictionary& defines) {
    const std::string KEYWORD{ "//<embed:" };

    std::ifstream in{ file };
    if (!in.is_open()) {
        return {};
    }

    std::string line;
    std::stringstream preprocessed;
    while (std::getline(in, line)) {
        if (line.find(KEYWORD) == 0) {
            const auto key{ line.substr(KEYWORD.size()) };
            const auto value{ defines.find(key) };
            if (value != defines.end()) {
                preprocessed << value->second << "\n";
            } else {
                std::cerr << "Define '" << key << "' not found\n";
                preprocessed << line << "\n";
            }
        } else {
            preprocessed << line << "\n";
        }
    }
    const auto result{ preprocessed.str() };

    Bytes content(result.size(), 0);
    std::copy(result.begin(), result.end(), content.begin());

    return content;
}

void ToHex(std::ostream& out, const Bytes& data) {
    auto counter{ 0 };
    for (const char c : data) {
        if (counter == 0) {
            out << "\t\t\"";
        }

        out << "\\x" << (std::hex) << static_cast<unsigned int>(static_cast<uint8_t>(c));

        if (++counter == 32) {
            out << "\"\n";
            counter = 0;
        }
    }

    if (counter > 0) {
        out << "\"";
    }

    out << ",\n";
    out << (std::dec);
}

void DumpHeader(std::ostream& out, std::string_view variable, std::string_view name, const Bytes& data) {
    out << "\tconstexpr ugine::tools::EmbeddedFile " << variable << " {\n";
    out << "\t\t\"" << name << "\",\n";
    ToHex(out, data);
    out << "\t\t" << data.size() << ",\n";
    out << "\t};\n";
}

void DumpSplitHeader(std::ostream& out, std::string_view variable) {
    out << "extern const ugine::tools::EmbeddedFile " << variable << ";\n";
}

void DumpSplitCpp(std::ostream& out, std::string_view variable, std::string_view name, const Bytes& data) {
    out << "const ugine::tools::EmbeddedFile " << variable << " {\n";
    out << "\t\"" << name << "\",\n";
    ToHex(out, data);
    out << "\t\t" << data.size() << ",\n";
    out << "\t};\n";
}

int main(int argc, char* argv[]) {
    // TODO: Arg parsing lib.
    if (argc < 5) {
        std::cerr << "Missing arguments.\n";
        std::cerr << "  Usage: " << argv[0] << " <input_file> <output_folder> <output_header> <variable_name>\n";
        return -1;
    }

    // TODO: namespace argument

    bool genCpp{ true };

    int numArgs{ 1 };
    const std::filesystem::path input{ argv[numArgs++] };
    const std::filesystem::path destination{ argv[numArgs++] };
    const std::filesystem::path header{ argv[numArgs++] };
    const std::string variableName{ argv[numArgs++] };
    bool verbose{};
    if (argc > numArgs) {
        verbose = strcmp(argv[numArgs++], "-v") == 0;
    }

    const auto destinationHeaderPath{ destination / header };

    if (verbose) {
        std::cout << "Embeding file '" << destinationHeaderPath << "'...\n";
    }

    Dictionary defines;
    for (; numArgs < argc; ++numArgs) {
        std::string_view arg{ argv[numArgs] };
        if (arg.size() > 2 && arg[0] == '-' && arg[1] == 'D') {
            const auto pos{ arg.find("=") };
            if (pos != std::string::npos) {
                const auto key{ arg.substr(2, pos - 2) };
                const auto value{ arg.substr(pos + 1) };

                defines.insert(std::make_pair(key, value));
            }
        }
    }

    std::filesystem::create_directories(destination);

    const auto inputFile{ input.filename().string() };

    const auto content{ defines.empty() ? ReadContent(input) : ReadContentPreprocess(input, defines) };
    if (content.empty()) {
        std::cerr << "Empty file: " << input << "\n";
        return -1;
    }

    if (genCpp) {
        std::ofstream headerOut{ destinationHeaderPath };
        headerOut << "// Generated file, do not modify.\n";
        headerOut << "#pragma once\n\n";
        headerOut << "#include <ugineTools/Embed.h>\n\n";

        DumpSplitHeader(headerOut, variableName);

        auto cppOutput{ header.filename() };
        cppOutput.replace_extension(".cpp");
        const auto destinationCppPath{ destination / cppOutput };

        std::ofstream cppOut{ destinationCppPath };
        cppOut << std::format("#include \"{}\"\n\n", header.filename().string());

        DumpSplitCpp(cppOut, variableName, inputFile, content);

    } else {
        std::ofstream headerOut{ destinationHeaderPath };
        headerOut << "// Generated file, do not modify.\n";
        headerOut << "#pragma once\n\n";
        DumpHeader(headerOut, variableName, inputFile, content);
    }

    return 0;
}