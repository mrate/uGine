#include <gtest/gtest.h>

#include <ugine/Memory.h>
#include <ugine/String.h>
#include <ugine/Vector.h>

#include <ugine/Serialization.h>

#include <bitsery/bitsery.h>

using namespace ugine;

struct SerializedStruct {
    String name;
    Vector<u8> data;
};

void serialize(auto& s, SerializedStruct& str) {
    s.text1b(str.name, 64);
    // TODO:
    //s.container(str.data, 16);
}

TEST(Serialization, BasicTypes) {
    Vector<u8> out;

    {
        SerializedStruct str{};
        str.name = "I'm a serialized struct!";
        str.data = { 1, 2, 3 };

        const auto count{ bitsery::quickSerialization(OutputAdapter{ out }, str) };
        ASSERT_GT(count, 0);
    }

    {
        SerializedStruct str{};

        auto state{ bitsery::quickDeserialization(InputAdapter{ out.Data(), out.Size() }, str) };

        ASSERT_EQ("I'm a serialized struct!", str.name);
    }
}