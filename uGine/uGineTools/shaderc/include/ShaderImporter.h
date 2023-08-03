#pragma once

#include <filesystem>
#include <map>
#include <string>
#include <vector>

class ShaderImporter {
public:
    struct Options {
        std::filesystem::path inputFile;
        std::filesystem::path outputFile;

        std::filesystem::path intermediateDir;

        std::vector<std::filesystem::path> includeDirs;
        bool warningsAsErrors{};
        bool optimizeForPerf{};
        bool generateDebugInfo{};
        bool verbose{};
        bool saveSteps{};
    };

    struct ShaderError {
        std::string message;
        std::filesystem::path filename;
        int errorLine{};
        int errorChar{};
        std::string shaderProcessedSource;
        std::map<std::string, std::string> defines;
    };

    bool Import(const ShaderImporter::Options& options);

    const std::string& Error() const { return error_; }
    const std::vector<ShaderError>& ShaderErrors() const { return shaderErrors_; }

private:
    std::string error_;
    std::vector<ShaderError> shaderErrors_;
};
