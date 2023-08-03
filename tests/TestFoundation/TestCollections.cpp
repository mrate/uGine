#include <gtest/gtest.h>

#include <ugine/Vector.h>

using namespace ugine;

TEST(Vector, Basic) {
    HeapAllocator heap;
    CountedAllocator allocator{ heap };

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

    Vector<u8> copy{ test.Clone() };
}

TEST(Vector, EraseReorder) {
    HeapAllocator heap;
    CountedAllocator allocator{ heap };

    {
        Vector<u8> test{ { 1, 2, 3, 4, 5 }, allocator };

        ASSERT_EQ(5, test.Size());

        test.EraseReorder(3);

        ASSERT_EQ(4, test.Size());

        ASSERT_EQ(1, test[0]);
        ASSERT_EQ(2, test[1]);
        ASSERT_EQ(5, test[2]);
        ASSERT_EQ(4, test[3]);
    }

    {
        Vector<u8> test{ { 1, 2, 3, 4, 5 }, allocator };

        ASSERT_EQ(5, test.Size());

        test.EraseIfReorder([](auto i) { return i < 3; });

        ASSERT_EQ(3, test.Size());

        ASSERT_EQ(5, test[0]);
        ASSERT_EQ(4, test[1]);
        ASSERT_EQ(3, test[2]);
    }
}

TEST(Vector, Initializer) {
    HeapAllocator heap;
    CountedAllocator allocator{ heap };

    {
        Vector<u8> test{ 1, 2, 3, 4, 5 };

        ASSERT_FALSE(test.Empty());
        ASSERT_EQ(5, test.Size());
        ASSERT_EQ(1, test[0]);
        ASSERT_EQ(2, test[1]);
        ASSERT_EQ(3, test[2]);
        ASSERT_EQ(4, test[3]);
        ASSERT_EQ(5, test[4]);
    }

    {
        Vector<u8> test{ 1, 2, 3, 4, 5 };

        ASSERT_FALSE(test.Empty());
        ASSERT_EQ(5, test.Size());
        ASSERT_EQ(1, test[0]);
        ASSERT_EQ(2, test[1]);
        ASSERT_EQ(3, test[2]);
        ASSERT_EQ(4, test[3]);
        ASSERT_EQ(5, test[4]);
    }
}

TEST(Vector, HybridVector) {
    HeapAllocator heap;
    CountedAllocator allocator{ heap };

    HybridVector<u8, 16> test{ allocator };
    ASSERT_EQ(0, allocator.Count());

    test.PushBack(1);
    test.PushBack(2);
    test.PushBack(3);

    ASSERT_EQ(3, test.Size());
    ASSERT_FALSE(test.Empty());
    ASSERT_EQ(16, test.Capacity());
    ASSERT_EQ(0, allocator.Count());

    ASSERT_EQ(1, test[0]);
    ASSERT_EQ(2, test[1]);
    ASSERT_EQ(3, test[2]);

    test.PushBack(4);
    test.PushBack(5);
    test.PushBack(6);
    test.PushBack(7);
    test.PushBack(8);
    test.PushBack(9);
    test.PushBack(10);
    test.PushBack(11);
    test.PushBack(12);
    test.PushBack(13);
    test.PushBack(14);
    test.PushBack(15);
    test.PushBack(16);

    ASSERT_EQ(16, test.Size());
    ASSERT_FALSE(test.Empty());
    ASSERT_EQ(16, test.Capacity());
    ASSERT_EQ(0, allocator.Count());

    test.PushBack(17);

    ASSERT_EQ(1, test[0]);
    ASSERT_EQ(2, test[1]);
    ASSERT_EQ(3, test[2]);

    ASSERT_EQ(17, test.Size());
    ASSERT_FALSE(test.Empty());
    ASSERT_TRUE(allocator.Count() > 0);

    for (int i{}; i < 32; ++i) {
        test.PushBack(i);
    }
}

TEST(Vector, CopyVector) {
    HeapAllocator heap;
    StackTraceAllocator allocator{ heap };

    Vector<int> v1{ { 1, 2, 3, 4, 5, 6 }, allocator };
    Vector<int> v2{ v1 };
    Vector<int> v3{ v1.Clone() };
    auto v4{ v3.Clone() };
    auto v5{ std::move(v4) };

    //v2 = v1;
}
