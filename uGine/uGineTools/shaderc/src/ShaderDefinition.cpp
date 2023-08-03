#include "ShaderDefinition.h"

#include <fstream>

bool Parse(const std::filesystem::path& file, ShaderFileDefinition& output) {
    try {
        nlohmann::json json;

        {
            std::ifstream in{ file };
            in >> json;
        }

        output = json.get<ShaderFileDefinition>();
        return true;
    } catch (const std::exception& /*ex*/) {
        return false;
    }
}