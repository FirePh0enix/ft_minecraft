#pragma once

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

class ThreadPool
{
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency() - 2);
    ~ThreadPool();

    size_t size() const { return m_threads.size(); }

    /// Starts an asynchronous task.
    void submit(std::function<void(std::stop_token)> task);

private:
    std::vector<std::jthread> m_threads;

    std::mutex m_mutex;
    std::deque<std::function<void(std::stop_token)>> m_tasks;

    std::condition_variable m_cv;
    bool m_should_stop = false;

    void thread_worker(std::stop_token token);
};
