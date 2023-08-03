#include <gtest/gtest.h>

#include <ugine/Path.h>
#include <ugine/FileSystem.h>

#include <filesystem>

using namespace ugine;

TEST(Path, Basic) {
    {
        Path p1{};
        ASSERT_EQ(true, p1.IsRelative());
        ASSERT_EQ(false, p1.IsAbsolute());
        ASSERT_EQ(false, p1.HasParentPath());
        ASSERT_EQ(true, p1.Empty());
        ASSERT_EQ(true, p1.Filename().Empty());
        ASSERT_EQ(true, p1.Stem().Empty());
        ASSERT_EQ(true, p1.Extension().Empty());
    }

    {
        Path p2{ "dir/file" };
        ASSERT_EQ(true, p2.IsRelative());
        ASSERT_EQ(false, p2.IsAbsolute());
        ASSERT_EQ(true, p2.HasParentPath());
        ASSERT_EQ("dir", p2.ParentPath().String());
        ASSERT_EQ("file", p2.Filename());
        ASSERT_EQ("file", p2.Stem());
        ASSERT_EQ(true, p2.Extension().Empty());
    }

    {
        Path p3{ "dir/file.ext" };
        ASSERT_EQ(true, p3.IsRelative());
        ASSERT_EQ(false, p3.IsAbsolute());
        ASSERT_EQ(true, p3.HasParentPath());
        ASSERT_EQ("dir", p3.ParentPath().String());
        ASSERT_EQ("file.ext", p3.Filename());
        ASSERT_EQ("file", p3.Stem());
        ASSERT_EQ(".ext", p3.Extension());
    }

    {
        Path p4{ "d:/dir/file.ext" };
        ASSERT_EQ(false, p4.IsRelative());
        ASSERT_EQ(true, p4.IsAbsolute());
        ASSERT_EQ(true, p4.HasParentPath());
        ASSERT_EQ("d:/dir", p4.ParentPath().String());
        ASSERT_EQ("file.ext", p4.Filename());
        ASSERT_EQ("file", p4.Stem());
        ASSERT_EQ(".ext", p4.Extension());

        Path p5{ p4 };
        p5.ReplaceExtension(".bat");
        ASSERT_EQ(false, p5.IsRelative());
        ASSERT_EQ(true, p5.IsAbsolute());
        ASSERT_EQ(true, p5.HasParentPath());
        ASSERT_EQ("d:/dir", p5.ParentPath().String());
        ASSERT_EQ("file.bat", p5.Filename());
        ASSERT_EQ("file", p5.Stem());
        ASSERT_EQ(".bat", p5.Extension());
    }

    {
        Path p6{ Path{} / "test" };
        ASSERT_EQ(true, p6.IsRelative());
        ASSERT_EQ(false, p6.IsAbsolute());
        ASSERT_EQ(false, p6.HasParentPath());
        ASSERT_EQ(".", p6.ParentPath().String());
        ASSERT_EQ("test", p6.Filename());
        ASSERT_EQ("test", p6.Stem());
        ASSERT_EQ("test", p6.String());
    }

    {
        for (auto it : std::filesystem::directory_iterator{ "." }) {
            const auto path{ it.path() };
            const auto isDir{ it.is_directory() };
            const auto isReg{ it.is_regular_file() };

            std::cout << "Path: " << path << "\n";
        }

        for (auto it : DirectoryIterator{ Path{ "." } }) {
            const auto path{ it.Path() };
            const auto isDir{ it.IsDirectory() };
            const auto isReg{ it.IsRegularFile() };

            std::cout << "Path: " << path.Data() << "\n";
        }
    }
}
