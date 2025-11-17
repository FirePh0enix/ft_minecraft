#include "Core/StringView.hpp"

StringView StringView::slice(size_t offset, size_t length) const
{
    // TODO: Check bounds
    return StringView(m_data + offset, length);
}

bool StringView::starts_with(const StringView& prefix) const
{
    return prefix.m_size <= m_size ? std::memcmp(m_data, prefix.c_str(), prefix.size()) == 0 : false;
}
