#include "Core/StringView.hpp"
#include "Core/Containers/Vector.hpp"
#include "Core/String.hpp"

StringView::StringView(const String& string)
    : m_data(string.data()), m_size(string.size())
{
}

bool StringView::contains(char c) const
{
    for (size_t index = 0; index < m_size; index++)
        if (m_data[index] == c)
            return true;
    return false;
}

bool StringView::starts_with(const StringView& prefix) const
{
    return prefix.m_size <= m_size ? std::memcmp(m_data, prefix.data(), prefix.size()) == 0 : false;
}

StringView StringView::slice(size_t offset, size_t length) const
{
    // TODO: Check bounds
    return StringView(m_data + offset, length);
}

Vector<StringView> StringView::split(StringView delim) const
{
    Vector<StringView> views;
    size_t prev_index = 0;
    size_t index = 0;

    while (index + delim.size() < m_size)
    {
        if (std::memcmp(m_data, delim.data(), delim.size()) == 0)
        {
            EXPECT(views.append(slice(prev_index, index - prev_index)));
            prev_index = index;
            index += delim.size();
        }
        else
        {
            index += 1;
        }
    }

    EXPECT(views.append(slice(prev_index)));
    return views;
}
