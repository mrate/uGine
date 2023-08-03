#pragma once

#include <ugine/String.h>

// TODO: hash
#include <functional>

namespace ugine {

class Path {
public:
    // TODO: MAX_PATH
    static constexpr size_t MaxLength{ 260 };
    static const char Separator;

    static Path RelativeDirectory(const Path& p1, const Path& p2);

    Path() = default;
    Path(const Path& other);
    Path(Path&& other);
    Path(StringView path);
    ~Path() = default;

    Path& operator=(const Path& other);
    Path& operator=(Path&& other);

    UGINE_FORCE_INLINE Path ParentPath() const { return parent_.Empty() ? Path{ "." } : Path{ parent_ }; }
    UGINE_FORCE_INLINE bool HasParentPath() const { return !parent_.Empty(); }
    UGINE_FORCE_INLINE StringView Filename() const { return fileName_; }
    UGINE_FORCE_INLINE StringView Extension() const { return extension_; }
    UGINE_FORCE_INLINE StringView Stem() const { return stem_; }

    void ReplaceExtension(StringView extension);

    UGINE_FORCE_INLINE bool IsAbsolute() const { return isAbsolute_; }
    UGINE_FORCE_INLINE bool IsRelative() const { return !isAbsolute_; }

    UGINE_FORCE_INLINE Path operator/(const Path& other) const { return *this / other.path_; }

    Path operator/(StringView path) const;

    UGINE_FORCE_INLINE bool Empty() const { return path_.Empty(); }
    UGINE_FORCE_INLINE StringView String() const { return StringView{ path_ }; }

    UGINE_FORCE_INLINE const char* Data() const { return path_.Data(); }

    UGINE_FORCE_INLINE u64 Hash() const { return StringID{ path_ }.Value(); }

    friend bool operator==(const Path& a, const Path& b);
    friend bool operator!=(const Path& a, const Path& b) { return !(a == b); }
    friend bool operator<(const Path& a, const Path& b);

private:
    void Update();

    StaticString<MaxLength> path_;
    StringView fileName_;
    StringView extension_;
    StringView parent_;
    StringView stem_;

    bool isAbsolute_{};
};

} // namespace ugine

namespace std {
template <> struct hash<ugine::Path> {
    size_t operator()(const ugine::Path& path) const noexcept { return path.Hash(); }
};
} // namespace std