#pragma once

#include <cstdint>
#include <initializer_list>
#include <optional>

namespace PortalPattern
{
struct Match
{
    int64_t x, y, z;
    bool along_z;
};

// Match a 4x5 vertical frame containing a 2x3 opening. Corners are optional.
// The anchor can be any frame cell when activating, or any interior cell when validating.
template <typename Read>
std::optional<Match> find(int64_t x, int64_t y, int64_t z, uint32_t frame,
                          uint32_t portal, bool active, Read read)
{
    for (bool along_z : {false, true})
        for (int ay = 0; ay < 5; ++ay)
            for (int ax = 0; ax < 4; ++ax)
            {
                const bool interior = ax > 0 && ax < 3 && ay > 0 && ay < 4;
                if (active && !interior)
                    continue;
                Match origin{.x = x - (along_z ? 0 : ax), .y = y - ay, .z = z - (along_z ? ax : 0), .along_z = along_z};
                bool matches = true;
                for (int dy = 0; dy < 5 && matches; ++dy)
                    for (int dx = 0; dx < 4; ++dx)
                    {
                        const bool inside = dx > 0 && dx < 3 && dy > 0 && dy < 4;
                        const bool corner = (dx == 0 || dx == 3) && (dy == 0 || dy == 4);
                        if (corner)
                            continue;
                        const uint32_t block = read(origin.x + (along_z ? 0 : dx), origin.y + dy,
                                                    origin.z + (along_z ? dx : 0));
                        if (inside ? (active ? block != portal : block != 0 && block != portal) : block != frame)
                        {
                            matches = false;
                            break;
                        }
                    }
                if (matches)
                    return origin;
            }
    return std::nullopt;
}
} // namespace PortalPattern
