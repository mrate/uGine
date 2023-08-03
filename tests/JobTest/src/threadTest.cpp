#include <ugine/Fiber.h>
#include <ugine/Thread.h>
#include <ugine/Ugine.h>

#include <chrono>
#include <iostream>
#include <thread>

void runThread() {
    std::cout << "Thread begin..." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(2));

    std::cout << "Thread end." << std::endl;
}

void testThreads() {
    using namespace ugine;

    HeapAllocator allocator;

    Thread t1{ "MyThread1", [] { runThread(); }, Thread::Priority::Normal, allocator };
    Thread t2{ "MyThread2", [] { runThread(); }, Thread::Priority::Normal, allocator };

    UGINE_ASSERT(t1.Joinable());
    t1.Join();
    UGINE_ASSERT(!t1.Joinable());

    UGINE_ASSERT(t2.Joinable());
    t2.Join();
    UGINE_ASSERT(!t2.Joinable());

    Thread x;
    UGINE_ASSERT(!x.Joinable());
}