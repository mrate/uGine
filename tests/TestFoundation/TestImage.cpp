#include <ugine/Image.h>

using namespace ugine;

void TestImage() {
    HeapAllocator heap;
    CountedAllocator callocator{ heap };
    StackTraceAllocator allocator{ callocator };

    Image image{ allocator };
    image = Image{ 100, 200, 4, 1, allocator };

    Image image2{ std::move(image) };

    Vector<Image> images{ allocator };
    images.PushBack(image2);

    Vector<Image> copy{ images.Clone() };
    UGINE_ASSERT(copy[0].Width() == 100);
    UGINE_ASSERT(copy[0].Height() == 200);
}