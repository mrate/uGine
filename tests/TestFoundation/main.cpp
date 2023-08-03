//#include <ugine/Image.h>
//#include <ugine/Log.h>
//#include <ugine/Memory.h>
//#include <ugine/Scheduler.h>
//#include <ugine/String.h>
//#include <ugine/Vector.h>
//
//#include <iostream>
//
//using namespace ugine;
//
//struct TlsData {
//    int value{};
//};
//
//TlsData& Tls() {
//    static int cnt{};
//    static thread_local TlsData data{ cnt++ };
//    return data;
//}
//
//class TestTask : public TaskSet {
//public:
//    void ExecuteRange(TaskSetPartition range_, uint32_t threadnum_) override {
//        std::cout << "Task on thread [" << threadnum_ << "] - tls=" << Tls().value << "\n";
//
//        std::this_thread::sleep_for(std::chrono::seconds(2));
//
//        //auto memory{ IAllocator::Default().Alloc(128) };
//
//        std::cout << "Task on thread [" << threadnum_ << "] - tls=" << Tls().value << "\n";
//    }
//};
//
//void TestScheduler() {
//    Scheduler scheduler{ 3 };
//
//    std::array<TestTask, 10> tasks;
//
//    for (auto& task : tasks) {
//        scheduler.Schedule(&task);
//    }
//
//    scheduler.ShutDown();
//}
//
//int main(int argc, char* argv[]) {
//    InitLogger();
//
//    TestStrings();
//    TestVector();
//    TestHybridVector();
//    TestStackTraceAllocator();
//    TestImage();
//    TestScheduler();
//
//    HashedString h1{ "some hased string" };
//    HashedString h2{ "some hased string"_hs };
//    const auto data{ h1.data() };
//    const auto val{ h2.value() };
//
//    return 0;
//}

#include <gtest/gtest.h>

int main(int argc, char* argv[]) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}