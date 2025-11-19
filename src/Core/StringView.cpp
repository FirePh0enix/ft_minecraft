#include "Core/StringView.hpp"
#include "Core/String.hpp"

StringView::StringView(const String& string)
    : m_data(string.data()), m_size(string.size())
{
}

StringView StringView::slice(size_t offset, size_t length) const
{
    // TODO: Check bounds
    return StringView(m_data + offset, length);
}

bool StringView::starts_with(const StringView& prefix) const
{
    return prefix.m_size <= m_size ? std::memcmp(m_data, prefix.data(), prefix.size()) == 0 : false;
}
