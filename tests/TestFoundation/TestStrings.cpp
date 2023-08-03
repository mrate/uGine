#include <gtest/gtest.h>

#include <ugine/String.h>

using namespace ugine;

constexpr StringID HashTest1{ "test" };
constexpr StringID HashTest2{ "test" };
constexpr StringID HashTest3{ "test1" };
constexpr auto HashTest4{ "test1"_hs };

static_assert(HashTest1.Value() == 18007334074686647077);
static_assert(HashTest1 == HashTest2);
static_assert(HashTest1 != HashTest3);
static_assert(HashTest3 == HashTest4);
static_assert(HashTest1 != HashTest4);

TEST(String, Basic) {

    HeapAllocator heap;
    CountedAllocator allocator{ heap };

    {
        String empty{ allocator };
        ASSERT_TRUE(empty.Empty());
        ASSERT_EQ(0, empty.Size());

        empty = "Something";
        ASSERT_FALSE(empty.Empty());
        ASSERT_EQ(9, empty.Size());

        empty = "Something even bigger than yourself";
        ASSERT_FALSE(empty.Empty());
        ASSERT_EQ(35, empty.Size());

        String other{ empty };
        ASSERT_FALSE(other.Empty());
        ASSERT_EQ(35, other.Size());

        ASSERT_EQ(empty, other);
        ASSERT_EQ("Something even bigger than yourself", other);

        other = "test";
        ASSERT_FALSE(other.Empty());
        ASSERT_EQ(4, other.Size());

        ASSERT_TRUE(other != empty);
        ASSERT_TRUE(other != "Something even bigger than yourself");

        other.Clear();
        ASSERT_TRUE(other.Empty());
        ASSERT_EQ(0, other.Size());

        other = std::move(empty);
        ASSERT_FALSE(other.Empty());
        ASSERT_EQ(35, other.Size());
        ASSERT_EQ("Something even bigger than yourself", other);

        ASSERT_TRUE(empty.Empty());
        ASSERT_EQ(0, empty.Size());

        auto ss{ other.SubStr(10, 4) };
        ASSERT_EQ(4, ss.Size());

        String substr{ ss, allocator };
        ASSERT_EQ(4, substr.Size());
        ASSERT_FALSE(substr.Empty());
        ASSERT_EQ("even", substr);
    }

    {
        StaticString<4> test{};
        ASSERT_TRUE(test.Empty());
        ASSERT_EQ(0, test.Size());
        ASSERT_EQ(test.SIZE, test.Limit());

        test = "test";
        ASSERT_EQ(4, test.Size());
        ASSERT_FALSE(test.Empty());

        test = String{ "something longer", allocator };
        ASSERT_EQ(4, test.Size());
        ASSERT_FALSE(test.Empty());
        ASSERT_EQ("some", test);

        test = "wtf";
        ASSERT_EQ(3, test.Size());
        ASSERT_FALSE(test.Empty());
        ASSERT_EQ("wtf", test);

        test = String{ "let's dance", allocator }.SubStr(6, 5);
        ASSERT_EQ(4, test.Size());
        ASSERT_FALSE(test.Empty());
        ASSERT_EQ("danc", test);
    }
}

TEST(String, StringView) {
    const char* text1 = "Test";
    const char text2[] = "Chest";

    StringView sv1{ text1 };
    ASSERT_EQ(4, sv1.Size());
    ASSERT_EQ(text1, sv1.Data());

    StringView sv2{ text2 };
    ASSERT_EQ(5, sv2.Size());
    ASSERT_EQ(text2, sv2.Data());

    StringView sv3{ text1, 2 };
    ASSERT_EQ(2, sv3.Size());
}
