#pragma once

#include "Core/Types.hpp"

#include <map>
#include <sstream>

template <typename... _Args>
class BasicFormatString
{
public:
    consteval BasicFormatString(const char *str)
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

template <typename... Args>
using FormatString = BasicFormatString<std::type_identity_t<Args>...>;

class FormatContext
{
public:
    FormatContext(std::basic_streambuf<char> *buf)
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

    void write_str(const char *s)
    {
        m_buf->sputn(s, (std::streamsize)strlen(s));
    }

private:
    std::basic_streambuf<char> *m_buf;
};

template <class _Buf, class... _Args>
void format_to(_Buf buf, FormatString<_Args...> fmt, _Args&&...args);

template <class _Buf>
void format_to(_Buf buf, FormatString<> fmt);

template <typename T>
struct Formatter
{
    void format(const T& value, FormatContext& ctx) const
    {
        std::stringstream ss;
        ss << value;

        std::string s = ss.str();
        ctx.write_str(s.data(), (std::streamsize)s.size());
    }
};

template <>
struct Formatter<const char *>
{
    void format(const char *& value, FormatContext& ctx) const
    {
        ctx.write_str(value, (std::streamsize)std::strlen(value));
    }
};

template <const size_t length>
struct Formatter<const char[length]>
{
    void format(const char value[length], FormatContext& ctx) const
    {
        ctx.write_str(value, length - 1);
    }
};

template <>
struct Formatter<std::string>
{
    void format(const std::string& s, FormatContext& ctx) const
    {
        ctx.write_str(s.data(), (std::streamsize)s.size());
    }
};

template <>
struct Formatter<bool>
{
    void format(const bool& s, FormatContext& ctx) const
    {
        ctx.write_str(s ? "true" : "false");
    }
};

template <typename T, const size_t size>
struct Formatter<std::array<T, size>>
{
    void format(const std::array<T, size>& array, FormatContext& ctx) const
    {
        ctx.write_str("{ ");

        for (size_t i = 0; i < array.size(); i++)
        {
            format_to(ctx.out(), "{}", array[i]);

            if (i + 1 < array.size())
                ctx.write_str(", ");
        }

        ctx.write_str(" }");
    }
};

template <typename T>
struct Formatter<std::vector<T>>
{
    void format(const std::vector<T>& vec, FormatContext& ctx) const
    {
        ctx.write_str("{ ");

        for (size_t i = 0; i < vec.size(); i++)
        {
            format_to(ctx.out(), "{}", vec[i]);

            if (i + 1 < vec.size())
                ctx.write_str(", ");
        }

        ctx.write_str(" }");
    }
};

template <typename K, typename V>
struct Formatter<std::map<K, V>>
{
    void format(const std::map<K, V>& map, FormatContext& ctx) const
    {
        ctx.write_str("{ ");

        size_t i = 0;

        for (const auto& iter : map)
        {
            format_to(ctx.out(), "{}: {}", iter->first, iter->second);

            if (i + 1 < map.size())
                ctx.write_str(", ");
            i++;
        }

        ctx.write_str(" }");
    }
};

template <typename T>
struct FormatBin
{
    T value;

    FormatBin(T value)
        : value(value)
    {
    }
};

template <class _Buf>
void __format_to(_Buf buf, FormatString<> fmt)
{
    FormatContext ctx(buf);
    ctx.write_str(fmt.get().data(), (std::streamsize)fmt.get().size());
}

template <class _Buf, size_t _ArgIndex, class... _Args>
void __format_to(_Buf buf, FormatString<_Args...> fmt, size_t offset, _Args&&...args)
{
    std::string_view str = fmt.get();
    size_t index = offset;
    bool inside_arg = false;

    FormatContext ctx(buf);

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

            Formatter<typeof(std::get<_ArgIndex>(std::forward_as_tuple(args...)))> f;
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
void format_to(_Buf buf, FormatString<_Args...> fmt, _Args&&...args)
{
    __format_to<_Buf, 0, _Args...>(buf, fmt, 0, std::forward<_Args>(args)...);
}

template <class _Buf>
void format_to(_Buf buf, BasicFormatString<> fmt)
{
    __format_to<_Buf>(buf, fmt, 0);
}

template <class... _Args>
std::string format(FormatString<_Args...> fmt, _Args&&...args)
{
    std::stringstream ss;
    __format_to<std::stringbuf *, 0, _Args...>(ss.rdbuf(), fmt, 0, std::forward<_Args>(args)...);
    return ss.str();
}

template <>
inline std::string format(BasicFormatString<> fmt)
{
    std::stringstream ss;
    __format_to<std::stringbuf *>(ss.rdbuf(), fmt);
    return ss.str();
}

template <typename T>
struct Formatter<FormatBin<T>>
{
    void format(const FormatBin<T>& value, FormatContext& ctx) const
    {
        if (value.value > 1024 * 1024 * 1024)
        {
            return format_to(ctx.out(), "{} GiB", (float)value.value / (float)(1024 * 1024 * 1024));
        }
        else if (value.value > 1024 * 1024)
        {
            return format_to(ctx.out(), "{} MiB", (float)value.value / (float)(1024 * 1024));
        }
        else if (value.value > 1024)
        {
            return format_to(ctx.out(), "{} KiB", (float)value.value / (float)(1024));
        }
        else
        {
            return format_to(ctx.out(), "{} B", (float)value.value);
        }
    }
};
