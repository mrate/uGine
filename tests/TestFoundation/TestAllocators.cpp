#include <gtest/gtest.h>

#include <ugine/Memory.h>
#include <ugine/String.h>
#include <ugine/Vector.h>

using namespace ugine;

TEST(Allocator, StackTrace) {
    HeapAllocator heap;
    CountedAllocator count{ heap };
    StackTraceAllocator allocator{ count, true };

    {
        Vector<u8> test{ allocator };

        for (int i{}; i < 10; ++i) {
            test.PushBack(i);
        }

        ASSERT_EQ(10, test.Size());
        ASSERT_FALSE(test.Empty());

        test.EraseFront(3);

        ASSERT_EQ(7, test.Size());
        ASSERT_FALSE(test.Empty());

        ASSERT_EQ(3, test.Front());
        ASSERT_EQ(9, test.Back());

        test.EraseBack(3);

        ASSERT_EQ(4, test.Size());
        ASSERT_FALSE(test.Empty());

        ASSERT_EQ(3, test.Front());
        ASSERT_EQ(6, test.Back());
    }

    auto mem{ allocator.Alloc(128) };
    ASSERT_EQ(1, allocator.ActiveAllocs());
    allocator.DumpAllocations();

    allocator.Free(mem);
}

class A {
public:
    virtual ~A() {}

    int value;
    Vector<u8> data;
};

class B {
public:
    virtual ~B() {}

    int other;
    String test;
};

class C : public A, public B {
public:
    float t;

    Vector<int> ints;
};

TEST(Allocator, UniquePtr) {
    HeapAllocator heap;
    StackTraceAllocator allocator{ heap, true };

    {
        auto a{ MakeUnique<A>(allocator) };
        a.Reset();
    }
    {
        auto b{ MakeUnique<B>(allocator) };
        b.Reset();
    }
    {
        auto c{ MakeUnique<C>(allocator) };
        c.Reset();
    }

    ASSERT_EQ(0, allocator.ActiveAllocs());
}