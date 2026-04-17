#pragma once

#include "Core/Containers/LocalVector.hpp"
#include "Core/Containers/Vector.hpp"

#include <condition_variable>
#include <mutex>

class ThreadPool
{
public:
    ThreadPool(size_t num_threads = std::thread::hardware_concurrency() - 1);
    ~ThreadPool();

    /**
     *  @brief Starts an asynchronous task.
     */
    Result<void> async(std::function<void()> task);

private:
    LocalVector<std::thread> m_threads;

    std::mutex m_queue_mutex;
    Vector<std::function<void()>> m_tasks;

    std::condition_variable m_cv;
    bool m_stop = false;

    void thread_worker();
};
