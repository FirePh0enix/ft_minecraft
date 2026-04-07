#include "World/Generator.hpp"

// ChunkPos Generator::pop_nearest_chunk(glm::vec3 position)
// {
//     ChunkPos chunk_pos = *m_load_orders.begin();
//     float lowest_distance_sq = INFINITY;

//     for (auto& pos : m_load_orders)
//     {
//         float distance_sq = glm::distance2(position, glm::vec3((double)pos.x * 16.0 + 8.0, 0, (double)pos.z * 16.0 + 8.0));

//         if (distance_sq < lowest_distance_sq)
//         {
//             lowest_distance_sq = distance_sq;
//             chunk_pos = pos;
//         }
//     }

//     m_load_orders.erase(chunk_pos);

//     return chunk_pos;
// }

// ChunkPos Generator::pop_farsest_chunk(glm::vec3 position)
// {
//     ChunkPos chunk_pos = *m_unload_orders.begin();
//     float lowest_distance_sq = INFINITY;

//     for (auto& pos : m_unload_orders)
//     {
//         float distance_sq = glm::distance2(position, glm::vec3((double)pos.x * 16.0 + 8.0, 0, (double)pos.z * 16.0 + 8.0));

//         if (distance_sq < lowest_distance_sq)
//         {
//             lowest_distance_sq = distance_sq;
//             chunk_pos = pos;
//         }
//     }

//     m_unload_orders.erase(chunk_pos);

//     return chunk_pos;
// }
