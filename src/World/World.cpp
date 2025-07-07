#include "World.hpp"
#include "Core/Format.hpp"
#include "Core/Logger.hpp"

World::World(Ref<Mesh> mesh, Ref<Material> material)
    : m_mesh(mesh), m_material(material)
{
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x / 16;
    int64_t chunk_z = z / 16;

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return BlockState();
    }

    Ref<Chunk> chunk = chunk_value.value();
    const size_t chunk_local_x = x > 0 ? (x % 16) : -(x % 16);
    const size_t chunk_local_z = z > 0 ? (z % 16) : -(z % 16);

    return chunk->get_block(chunk_local_x, y, chunk_local_z);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    int64_t chunk_x = x / 16;
    int64_t chunk_z = z / 16;

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.value();
    const size_t chunk_local_x = x > 0 ? (x % 16) : -(x % 16);
    const size_t chunk_local_z = z > 0 ? (z % 16) : -(z % 16);

    chunk->set_block(chunk_local_x, y, chunk_local_z, state);
    update_visibility(x, y, z, true);
}

void World::update_visibility(int64_t x, int64_t y, int64_t z, bool recurse)
{
    int64_t chunk_x = x / 16;
    int64_t chunk_z = z / 16;

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.value();
    const size_t chunk_local_x = x > 0 ? (x % 16) : -(x % 16);
    const size_t chunk_local_z = z > 0 ? (z % 16) : -(z % 16);

    chunk->compute_visibility(this, chunk_local_x, y, chunk_local_z);

    if (recurse)
    {
        update_visibility(x - 1, y, z, false);
        update_visibility(x + 1, y, z, false);

        update_visibility(x, y - 1, z, false);
        update_visibility(x, y + 1, z, false);

        update_visibility(x, y, z - 1, false);
        update_visibility(x, y, z + 1, false);
    }
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z) const
{
    return m_dims[0].get_chunk(x, z);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t z)
{
    return m_dims[0].get_chunk(x, z);
}

void World::set_render_distance(uint32_t distance)
{
    const uint32_t old_buffer_count = m_buffers.size();
    const uint32_t new_buffer_count = (distance * 2 + 1) * (distance * 2 + 1);

    std::lock_guard<std::mutex> lock(m_buffers_mutex);

    m_distance = distance;

    // Create or recreate instance buffers.
    m_buffers.resize(new_buffer_count);

    if (new_buffer_count <= old_buffer_count)
        return;

    for (size_t i = 0; i < new_buffer_count - old_buffer_count; i++)
    {
        auto buffer_result = RenderingDriver::get()->create_buffer(sizeof(BlockInstanceData) * Chunk::block_count, {.copy_dst = true, .vertex = true});
        ERR_EXPECT_C(buffer_result, "Failed to create instance buffer");
        m_buffers[i + old_buffer_count] = BufferInfo{.buffer = buffer_result.value(), .used = false};
    }

    info("{} bytes of VRAM allocated for chunk buffers", FormatBin(m_buffers.size() * sizeof(BlockInstanceData) * Chunk::block_count));
}

void World::encode_draw_calls(RenderGraph& graph, Camera& camera)
{
    ZoneScoped;

    std::lock_guard<std::mutex> lock(m_chunks_read_mutex);

    for (const auto& chunk_pair : m_dims[0])
    {
        AABB aabb = AABB(glm::vec3((float)chunk_pair.first.x * 16.0 + 8.0, 128.0, (float)chunk_pair.first.z * 16.0 + 8.0), glm::vec3(8.0, 128.0, 8.0));

        if (camera.frustum().contains(aabb))
        {
            const Ref<Buffer>& buffer = m_buffers[chunk_pair.second->get_buffer_id()].buffer;
            graph.add_draw(m_mesh, m_material, camera.get_view_proj_matrix(), chunk_pair.second->get_block_count(), buffer);
        }
    }
}

std::optional<size_t> World::acquire_buffer()
{
    std::lock_guard<std::mutex> lock(m_buffers_mutex);

    for (size_t i = 0; i < m_buffers.size(); i++)
    {
        auto& info = m_buffers[i];

        if (!info.used)
        {
            info.used = true;
            return i;
        }
    }

    return std::nullopt;
}

void World::free_buffer(size_t index)
{
    std::lock_guard<std::mutex> lock(m_buffers_mutex);
    m_buffers[index].used = false;
}
