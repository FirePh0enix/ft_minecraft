#pragma once

#include "types.hpp"

#include <fstream>
#include <sstream>
#include <string>

namespace lib
{

template <typename... _Args>
class format_string
{
public:
    consteval format_string(const char *str)
        : m_str(str)
    {
    }

    std::string_view get() const
    {
        return m_str;
    }

private:
    std::string_view m_str;
};

class format_context
{
public:
    format_context(std::basic_streambuf<char> *buf)
        : m_buf(buf)
    {
    }

    inline std::basic_streambuf<char> *out() const
    {
        return m_buf;
    }

    void write_char(char c)
    {
        m_buf->sputc(c);
        m_buf->pubsync();
    }

    void write_str(const char *s, std::streamsize len)
    {
        m_buf->sputn(s, len);
        m_buf->pubsync();
    }

private:
    std::basic_streambuf<char> *m_buf;
};

template <typename T>
struct formatter
{
    void format(const T& value, format_context& ctx) const
    {
        std::stringstream ss;
        ss << value;

        std::string s = ss.str();
        ctx.write_str(s.data(), (std::streamsize)s.size());
    }
};

template <>
struct formatter<const char *>
{
    void format(const char *& value, format_context& ctx) const
    {
        ctx.write_str(value, (std::streampos)std::strlen(value));
    }
};

template <const size_t length>
struct formatter<const char[length]>
{
    void format(const char value[length], format_context& ctx) const
    {
        ctx.write_str(value, length - 1);
    }
};

template <>
struct formatter<std::string>
{
    void format(const std::string& s, format_context& ctx) const
    {
        ctx.write_str(s.data(), s.size());
    }
};

template <class _Buf>
void __format_to(_Buf buf, format_string<> fmt)
{
    format_context ctx(buf);
    ctx.write_str(fmt.get().data(), (std::streamsize)fmt.get().size());
}

template <class _Buf, size_t _ArgIndex, class... _Args>
void __format_to(_Buf buf, format_string<type_identity_t<_Args>...> fmt, size_t offset, _Args&&...args)
{
    std::string_view str = fmt.get();
    size_t index = offset;
    bool inside_arg = false;

    format_context ctx(buf);

    for (; index < str.size(); index++)
    {
        if (index + 1 < str.size() && str[index] == '{' && str[index + 1] == '{')
        {
            index += 1;
        }
        else if (str[index] == '{')
        {
            inside_arg = true;
        }
        else if (index + 1 < str.size() && str[index] == '}' && str[index + 1] == '}')
        {
            index += 1;
        }
        else if (str[index] == '}')
        {
            inside_arg = false;

            formatter<typeof(std::get<_ArgIndex>(std::forward_as_tuple(args...)))> f;
            auto& value = std::get<_ArgIndex>(std::forward_as_tuple(args...));
            f.format(value, ctx);

            index++;
            break;
        }
        else if (!inside_arg)
        {
            ctx.write_char(str[index]);
        }
    }

    if constexpr (_ArgIndex + 1 < sizeof...(_Args))
    {
        __format_to<_Buf, _ArgIndex + 1>(buf, fmt, index, std::forward<_Args>(args)...);
    }
    else if (index < str.size())
    {
        __format_to<_Buf, _ArgIndex>(buf, fmt, index, std::forward<_Args>(args)...);
    }
}

template <class _Buf, class... _Args>
void format_to(_Buf buf, format_string<type_identity_t<_Args>...> fmt, _Args&&...args)
{
    __format_to<_Buf, 0, _Args...>(buf, fmt, 0, std::forward<_Args>(args)...);
}

template <class _Buf>
void format_to(_Buf buf, format_string<> fmt)
{
    __format_to<_Buf>(buf, fmt, 0);
}

template <class... _Args>
std::string format(format_string<type_identity_t<_Args>...> fmt, _Args&&...args)
{
    std::stringstream ss;
    __format_to<std::stringbuf *, 0, _Args...>(ss.rdbuf(), fmt, 0, std::forward<_Args>(args)...);
    return ss.str();
}

template <>
inline std::string format(format_string<> fmt)
{
    std::stringstream ss;
    __format_to<std::stringbuf *>(ss.rdbuf(), fmt);
    return ss.str();
}

} // namespace lib
