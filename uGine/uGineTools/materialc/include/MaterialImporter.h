#pragma once

#include <ugine/engine/gfx/asset/SerializedMaterial.h>

#include <string>
#include <vector>

namespace ugine::tools {

class MaterialImporter {
public:
    using Defines = std::map<std::string, std::string>;

    struct Error {
        String message;
        Path filename;
    };

    explicit MaterialImporter(const Path& path) noexcept;

    ugine::SerializedMaterial LoadMaterial(const Vector<Path>& includeDirs = {});

    const Vector<Error>& GetErrors() const { return errors_; }

private:
    Path path_;

    Vector<Error> errors_;
};

} // namespace ugine::tools