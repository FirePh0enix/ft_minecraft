#pragma once

template <typename Callback>
class Defer
{
public:
    Defer(Callback callback)
        : m_callback(callback)
    {
    }

    ~Defer()
    {
        m_callback();
    }

private:
    Callback m_callback;
};
