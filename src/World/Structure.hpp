#pragma once

#include "Block/Block.hpp"
#include "Core/StringView.hpp"

#include <yaml-cpp/yaml.h>

class Structure
{
public:
    static std::shared_ptr<Structure> load(const StringView& path);

    int64_t width() const { return m_width; }
    int64_t height() const { return m_height; }
    int64_t length() const { return m_length; }
    BlockState *blocks() const { return m_blocks; }

private:
    int64_t m_width;
    int64_t m_height;
    int64_t m_length;
    BlockState *m_blocks;
};
