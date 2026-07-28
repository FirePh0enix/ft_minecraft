#include "World/Gen.hpp"

#include "Engine.hpp"
#include "World/Registry.hpp"

void TreePass::init(GenDesc& desc)
{
    (void)desc;
    m_tree_struct = Engine::get().registry().get_struct("tree");
}

void TreePass::gen(const Gen& gen, std::shared_ptr<Chunk> chunk)
{
    float height = *gen.get_buffer<float>("height");

    const int64_t w = m_tree_struct->width();
    const int64_t h = m_tree_struct->height();
    const int64_t l = m_tree_struct->length();

    if (chunk->get_biomes()[7 + 7 * 16] != Biome::Plain)
        return;

    for (int64_t x = 0; x < w; x++)
        for (int64_t y = 0; y < h; y++)
            for (int64_t z = 0; z < l; z++)
            {
                int64_t cx = x;
                int64_t cy = int64_t(height) + y;
                int64_t cz = z;

                if (cx < 0 || cx > 15 || cy < 0 || cy > 255 || cz < 0 || cz > 15)
                    continue;

                BlockState state = m_tree_struct->blocks()[x + y * w + z * w * h];
                if (!state.is_air())
                    chunk->get_blocks()[cx + cy * 16 + cz * 16 * 256] = state;
            }
}
