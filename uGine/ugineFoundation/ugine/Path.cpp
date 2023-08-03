#include "Path.h"

//#if defined(_WIN32)
//#include <shlwapi.h>
//
//#pragma comment(lib, "shlwapi.lib")
//#endif

#include <filesystem>

namespace ugine {

const char Path::Separator{ '\\' };

Path Path::RelativeDirectory(const Path& from, const Path& to) {
    // TODO: Does something different.
    //Path result{};
    //PathRelativePathToA(result.path_.Data(), from.Data(), 0, to.Data(), FILE_ATTRIBUTE_DIRECTORY);
    //return result;

    return Path{ std::filesystem::relative(from.Data(), to.Data()).string() };
}

Path::Path(const Path& other)
    : path_{ other.path_ } {
    Update();
}

Path::Path(Path&& other)
    : path_{ std::move(other.path_) } {
    Update();
}

Path& Path::operator=(const Path& other) {
    path_ = other.path_;
    Update();

    return *this;
}

Path& Path::operator=(Path&& other) {
    path_ = std::move(other.path_);
    Update();

    return *this;
}

Path::Path(StringView path)
    : path_{ path } {
    Update();
}

void Path::ReplaceExtension(StringView extension) {
    int dot{ -1 };
    for (int i{ int(path_.Size() - 1) }; i >= 0; --i) {
        if (path_[i] == '.') {
            dot = i;
            break;
        }
    }

    if (dot >= 0) {
        path_.Resize(dot);
    }

    path_.Append(extension);
    Update();
}

Path Path::operator/(StringView path) const {
    Path newPath{ *this };
    if (!newPath.path_.Empty() && newPath.path_[newPath.path_.Size() - 1] != Separator) {
        newPath.path_.Append(Separator);
    }
    newPath.path_.Append(path);
    newPath.Update();
    return newPath;
}

void Path::Update() {
    // TODO: Make canonical by default.

    isAbsolute_ = false;
    int lastSeparator{ -1 };
    int lastDot{ -1 };

    for (int i{}; i < path_.Size(); ++i) {
        switch (path_[i]) {
        case ':': isAbsolute_ = true; break;
        case '\\':
            lastSeparator = i;
            path_[i] = Separator;
            break;
        case '/': lastSeparator = i; break;
        case '.':
            if (i == 0 || lastDot < i - 1) {
                lastDot = i;
            } else {
                lastDot = -1; // ".."
            }
            break;
        }
    }

    parent_ = lastSeparator < 0 ? StringView{} : StringView{ path_.Data(), size_t(lastSeparator) };
    fileName_ = lastSeparator < 0 ? StringView{ path_ } : StringView{ path_.Data() + lastSeparator + 1 };
    extension_ = lastDot < 0 ? StringView{} : StringView{ path_.Data() + lastDot };
    stem_ = lastDot < 0 ? fileName_ : StringView{ fileName_.Begin(), path_.Size() - lastDot };
}

bool operator==(const Path& a, const Path& b) {
    // TODO: [PATH] proper comparison
    const auto res{ a.path_ == b.path_ };
    return res;
}

bool operator<(const Path& a, const Path& b) {
    return strcmp(a.Data(), b.Data()) < 0;
}

} // namespace ugine
