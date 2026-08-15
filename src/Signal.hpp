#pragma once

#include <functional>
#include <vector>

template <typename... Args>
class Signal
{
public:
    void connect(std::function<void(Args...)> callback)
    {
        m_callbacks.push_back(callback);
    }

    void emit(Args... args)
    {
        for (auto f : m_callbacks)
            f(std::forward<Args>(args)...);
    }

private:
    std::vector<std::function<void(Args...)>> m_callbacks;
};
