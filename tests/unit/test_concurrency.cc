#include "myshell/concurrency/thread_pool.hh"

#include <gtest/gtest.h>

TEST(ThreadPoolTest, SubmitTask) {
    myshell::concurrency::ThreadPool pool(2);
    auto fut = pool.submit([]() { return 42; });

    EXPECT_EQ(fut.get(), 42);
}

TEST(ThreadPoolTest, MultipleConcurrentTasks) {
    myshell::concurrency::ThreadPool pool(4);
    std::vector<std::future<int>> futures;
    for (size_t i = 0; i < 20; ++i) {
        futures.push_back(pool.submit([val = static_cast<int>(i)]() { return val * val; }));
    }

    for (size_t i = 0; i < 20; ++i) {
        int val = static_cast<int>(i);
        EXPECT_EQ(futures[i].get(), val * val);
    }
}
