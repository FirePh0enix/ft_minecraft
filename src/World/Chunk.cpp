#include "World/Chunk.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

constexpr uint8_t north_mask = 1 << 0;
constexpr uint8_t south_mask = 1 << 1;
constexpr uint8_t west_mask = 1 << 2;
constexpr uint8_t east_mask = 1 << 3;
constexpr uint8_t top_mask = 1 << 4;
constexpr uint8_t bottom_mask = 1 << 5;

void Chunk::compute_full_visibility(const Ref<World>& world)
{
    ZoneScoped;

    std::optional<Ref<Chunk>> north = world->get_chunk(x(), z() - 1);
    std::optional<Ref<Chunk>> south = world->get_chunk(x(), z() + 1);
    std::optional<Ref<Chunk>> west = world->get_chunk(x() - 1, z());
    std::optional<Ref<Chunk>> east = world->get_chunk(x() + 1, z());

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = 0; y < Chunk::height; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                uint8_t visibility = 0;

                if (z == 0)
                {
                    if (!north.has_value() || !north.value()->has_block(x, y, 15))
                        visibility |= north_mask;
                }
                else if (!has_block(x, y, z - 1))
                {
                    visibility |= north_mask;
                }

                if (z == 15)
                {
                    if (!south.has_value() || !south.value()->has_block(x, y, 0))
                        visibility |= south_mask;
                }
                else if (!has_block(x, y, z + 1))
                {
                    visibility |= south_mask;
                }

                if (x == 0)
                {
                    if (!west.has_value() || !west.value()->has_block(15, y, z))
                        visibility |= west_mask;
                }
                else if (!has_block(x - 1, y, z))
                {
                    visibility |= west_mask;
                }

                if (x == 15)
                {
                    if (!east.has_value() || !east.value()->has_block(0, y, z))
                        visibility |= east_mask;
                }
                else if (!has_block(x + 1, y, z))
                {
                    visibility |= east_mask;
                }

                if (y == 255 || !has_block(x, y + 1, z))
                {
                    visibility |= top_mask;
                }

                if (y == 0 || !has_block(x, y - 1, z))
                {
                    visibility |= bottom_mask;
                }

                get_block_ref(x, y, z).generic.visibility = visibility;
            }
        }
    }
}

void Chunk::compute_visibility(const World *world, int64_t x, int64_t y, int64_t z)
{
    std::optional<Ref<Chunk>> north = world->get_chunk(this->x(), this->z() - 1);
    std::optional<Ref<Chunk>> south = world->get_chunk(this->x(), this->z() + 1);
    std::optional<Ref<Chunk>> west = world->get_chunk(this->x() - 1, this->z());
    std::optional<Ref<Chunk>> east = world->get_chunk(this->x() + 1, this->z());

    uint8_t visibility = 0;

    if (z == 0)
    {
        if (!north.has_value() || !north.value()->has_block(x, y, 15))
            visibility |= north_mask;
    }
    else if (!has_block(x, y, z - 1))
    {
        visibility |= north_mask;
    }

    if (z == 15)
    {
        if (!south.has_value() || !south.value()->has_block(x, y, 0))
            visibility |= south_mask;
    }
    else if (!has_block(x, y, z + 1))
    {
        visibility |= south_mask;
    }

    if (x == 0)
    {
        if (!west.has_value() || !west.value()->has_block(15, y, z))
            visibility |= west_mask;
    }
    else if (!has_block(x - 1, y, z))
    {
        visibility |= west_mask;
    }

    if (x == 15)
    {
        if (!east.has_value() || !east.value()->has_block(0, y, z))
            visibility |= east_mask;
    }
    else if (!has_block(x + 1, y, z))
    {
        visibility |= east_mask;
    }

    if (y == 255 || !has_block(x, y + 1, z))
    {
        visibility |= top_mask;
    }

    if (y == 0 || !has_block(x, y - 1, z))
    {
        visibility |= bottom_mask;
    }

    get_block_ref(x, y, z).generic.visibility = visibility;
    update_instance_buffer(world->get_buffer(get_buffer_id()));
}

void Chunk::update_instance_buffer(Ref<Buffer> buffer)
{
    ZoneScoped;

    std::vector<BlockInstanceData> instances;
    instances.reserve(Chunk::block_count);

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t z = 0; z < Chunk::width; z++)
        {
            for (int64_t y = 0; y < Chunk::height; y++)
            {
                BlockState state = get_block(x, y, z);
                if (state.id == 0)
                {
                    continue;
                }

                int64_t gx = m_x * 16 + x;
                int64_t gz = m_z * 16 + z;

                const Ref<Block>& block = BlockRegistry::get().get_block_by_id(state.id);
                if (!block)
                {
                    continue;
                }

                if (x < m_min_x)
                {
                    m_min_x = x;
                }
                if (x > m_max_x)
                {
                    m_max_x = x;
                }

                if (y < m_min_y)
                {
                    m_min_y = y;
                }
                if (y > m_max_y)
                {
                    m_max_y = y;
                }

                if (z < m_min_z)
                {
                    m_min_z = z;
                }
                if (z > m_max_z)
                {
                    m_max_z = z;
                }

                const std::array<uint32_t, 6> t = block->get_texture_ids();

                glm::uvec3 textures((t[0] & 0xFFFF) | (t[1] << 16), (t[2] & 0xFFFF) | (t[3] << 16), (t[4] & 0xFFFF) | (t[5] << 16));
                instances.push_back(BlockInstanceData{.position = glm::vec3((float)gx, (float)y, (float)gz), .textures = textures, .visibility = state.generic.visibility});
            }
        }
    }

    RenderingDriver::get()->update_buffer(buffer, View(instances).as_bytes(), 0);
    m_block_count = instances.size();
}
