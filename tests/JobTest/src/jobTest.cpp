#include <ugine/Jobs.h>
#include <ugine/Ugine.h>

#include <iostream>
#include <format>

using namespace ugine;

struct Animation {
    int value{};
    std::string name;
};

Animation g_animations[100];
JobScheduler g_jobs;

void UpdateAnimationJob(void* arg) {
    auto animation{ static_cast<Animation*>(arg) };

    std::cout << "Updating animation " << animation->name << std::endl;

    // TODO:
    for (int i{}; i < 0xffffff; ++i) {
        animation->value = i;
    }

    std::cout << "Updating animation " << animation->name << " done" << std::endl;
}

void UpdateAnimationsJob(void* arg) {
    std::cout << "UpdateAnimations started" << std::endl;

    Job jobs[100];

    JobCounter counter{ 100 };
    for (int i{}; i < 100; ++i) {
        jobs[i].func = UpdateAnimationJob;
        jobs[i].arg = &g_animations[i];
        jobs[i].counter = &counter;

        g_jobs.AddJob(jobs[i]);
    }

    g_jobs.Wait(&counter);

    std::cout << "UpdateAnimations finished" << std::endl;
}

void testJobs() {
    HeapAllocator allocator{};

    for (int i{}; i < 100; ++i) {
        g_animations[i].name = std::format("Animation #{}", i);
    }

    g_jobs.Init(allocator, Thread::HardwareConcurency());

    JobCounter counter{ 1 };

    Job job{
        .func = UpdateAnimationsJob,
        .arg = nullptr,
        .counter = &counter,
    };

    g_jobs.AddJob(job);
    g_jobs.Wait(&counter);

    std::this_thread::sleep_for(std::chrono::seconds(10));

    g_jobs.Shutdown();
}