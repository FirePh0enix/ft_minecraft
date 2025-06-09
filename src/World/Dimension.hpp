#pragma once

#include "World/Chunk.hpp"

class Dimension
{
public:
    Dimension()
    {
    }

    inline const std::vector<Chunk>& get_chunks() const
    {
        return m_chunks;
    }

    inline std::vector<Chunk>& get_chunks()
    {
        return m_chunks;
    }

private:
    std::vector<Chunk> m_chunks;
};
