#pragma once

#include <cstdint>

namespace ChunkStreaming
{
inline constexpr int64_t render_distance = 16;
// Mesh terrain before it crosses the visible horizon, including after turns.
inline constexpr int64_t preload_margin = 4;
inline constexpr int64_t load_distance = render_distance + preload_margin;
inline constexpr double prediction_seconds = 1.0;
inline constexpr double max_prediction_blocks = 160.0;
inline constexpr float fog_end = float(render_distance * 16 - 1);
} // namespace ChunkStreaming

struct WorldSettings
{
    uint64_t seed = 0;
    int64_t ocean_floor = 10;
    int64_t ocean_level = 48;
};
