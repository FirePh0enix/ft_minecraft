#include "Core/ThreadPool.hpp"

#include "Logger.hpp"
#include "Profiler.hpp"

#include <algorithm>
#include <mutex>

ThreadPool::ThreadPool(size_t num_threads)
{
    num_threads = std::max<size_t>(num_threads, 1);
    for (size_t i = 0; i < num_threads; i++)
    {
        m_threads.emplace_back([this](std::stop_token token)
                               { thread_worker(token); });
    }
}

ThreadPool::~ThreadPool()
{
    {
        std::lock_guard lock(m_mutex);
        m_should_stop = true;
    }

    for (auto& thread : m_threads)
    {
        thread.request_stop();
    }

    m_cv.notify_all();

    // Join before this object's mutex/condition variable are destroyed.
    // std::jthread only joins from its destructor, which runs after the other
    // members below have already been torn down (members are destroyed in
    // reverse declaration order).
    for (auto& thread : m_threads)
    {
        if (thread.joinable())
            thread.join();
    }
}

void ThreadPool::submit(std::function<void(std::stop_token)> task)
{
    // {
    //     std::lock_guard lock(m_mutex);
    //     m_tasks.push_back(task);
    // }

    m_tasks.enqueue(task);
    m_cv.notify_one();
}

void ThreadPool::thread_worker(std::stop_token token)
{
    TracySetThreadName("ThreadPool");

    while (!token.stop_requested())
    {
        TaskFn task;

        {
            std::unique_lock lock(m_mutex);
            m_cv.wait(lock, [this, token]
                      { return !m_tasks.empty() || m_should_stop || token.stop_requested(); });

            if (m_should_stop && m_tasks.empty())
                break;
            if (token.stop_requested())
                break;

            if (!m_tasks.try_dequeue(task))
                continue;
        }

        try
        {
            task(token);
        }
        catch (...)
        {
        }
    }

    debug("Thread {} exited normally", (void *)pthread_self());
}
