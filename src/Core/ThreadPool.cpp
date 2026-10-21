#include "Core/ThreadPool.hpp"

#include "Logger.hpp"
#include "Profiler.hpp"

#include <algorithm>
#include <mutex>

ThreadPool::ThreadPool(size_t num_threads)
{
    start(num_threads);
}

void ThreadPool::start(size_t num_threads)
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
    stop();
}

void ThreadPool::stop()
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

    // All task owners must remain alive until every worker has joined.
    for (auto& thread : m_threads)
    {
        if (thread.joinable())
            thread.join();
    }

    m_threads.clear();
    TaskFn task;
    while (m_tasks.try_dequeue(task))
        task = {};
}

void ThreadPool::reset()
{
    const size_t num_threads = size();
    stop();
    {
        std::lock_guard lock(m_mutex);
        m_should_stop = false;
    }
    start(num_threads);
}

void ThreadPool::submit(std::function<void(std::stop_token)> task)
{
    {
        // Synchronize with both shutdown and the worker's wait predicate.
        std::lock_guard lock(m_mutex);
        if (m_should_stop)
            return;
        m_tasks.enqueue(std::move(task));
    }
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

#ifdef __platform_windows
    debug("Thread {} exited normally", std::hash<std::thread::id>{}(std::this_thread::get_id()));
#else
    debug("Thread {} exited normally", (void *)pthread_self());
#endif
}
