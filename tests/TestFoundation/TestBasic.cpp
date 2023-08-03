#include <gtest/gtest.h>

#include <ugine/String.h>
#include <ugine/UUID.h>
#include <ugine/Utils.h>

using namespace ugine;

TEST(Basic, Utils) {
    enum Enum {
        ENUM_VALUE,
    };

    enum class ScopedEnum {
        Value,
    };

    static_assert(std::is_same_v<ArgType<int>, int>, "ArgType<int>");
    static_assert(std::is_same_v<ArgType<bool>, bool>, "ArgType<bool>");
    static_assert(std::is_same_v<ArgType<Enum>, Enum>, "ArgType<Enum>");
    static_assert(std::is_same_v<ArgType<ScopedEnum>, ScopedEnum>, "ArgType<ScopedEnum>");
    static_assert(std::is_same_v<ArgType<void>, void>, "ArgType<void>");
    static_assert(std::is_same_v<ArgType<nullptr_t>, nullptr_t>, "ArgType<nullptr_t>");
    static_assert(std::is_same_v<ArgType<String>, const String&>, "ArgType<String>");
}

TEST(Basic, UUID) {
    for (int i{}; i < 100; ++i) {
        auto id{ UUID::Generate() };

        auto str{ id.ToString() };
        auto fromStr{ UUID::FromString(str) };

        ASSERT_EQ(id, fromStr);
    }
}