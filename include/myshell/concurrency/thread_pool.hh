// include/myshell/concurrency/thread_pool.hh
#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <queue>
#include <stop_token>
#include <thread>
#include <type_traits>
#include <vector>

namespace myshell::concurrency {

class ThreadPool {
public:
    explicit ThreadPool(std::size_t num_threads = 0) {
        if (num_threads == 0) {
            num_threads = std::thread::hardware_concurrency();
            if (num_threads == 0)
                num_threads = 4;
        }

        workers_.reserve(num_threads);
        for (std::size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this](std::stop_token stop) { worker_loop(stop); });
        }
    }

    ~ThreadPool() {
        stop();
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args) -> std::future<std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            [f = std::forward<F>(f), ... args = std::forward<Args>(args)]() mutable {
                return f(args...);
            });

        std::future<return_type> res = task->get_future();
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            if (stopping_) {
                throw std::runtime_error("submit on stopped ThreadPool");
            }
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return res;
    }

    void stop() {
        {
            std::unique_lock<std::mutex> lock(queue_mtx_);
            stopping_ = true;
        }
        cv_.notify_all();
        for (auto& worker : workers_) {
            worker.request_stop();
        }
        workers_.clear();
    }

    [[nodiscard]] std::size_t thread_count() const noexcept {
        return workers_.size();
    }

private:
    void worker_loop(std::stop_token stop) {
        while (!stop.stop_requested()) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(queue_mtx_);
                cv_.wait(lock, [this, &stop]() {
                    return stopping_ || !tasks_.empty() || stop.stop_requested();
                });

                if ((stopping_ && tasks_.empty()) || stop.stop_requested()) {
                    return;
                }

                if (!tasks_.empty()) {
                    task = std::move(tasks_.front());
                    tasks_.pop();
                }
            }

            if (task) {
                task();
            }
        }
    }

    std::vector<std::jthread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mtx_;
    std::condition_variable cv_;
    bool stopping_{false};
};

}  // namespace myshell::concurrency
