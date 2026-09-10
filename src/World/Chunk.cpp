#include "World/Chunk.hpp"

#include "Block/Block.hpp"
#include "Engine.hpp"
#include "Render/Renderer.hpp"
#include "World/Registry.hpp"

#include <cstdint>

Chunk::Chunk(Dimension *dim, int64_t x, int64_t z)
    : m_dim(dim), m_x(x), m_z(z)
{
    m_blocks = new BlockState[block_count];
    m_biomes = new Biome[16 * 16];
    m_slices = new Slice[slice_count];

    // m_tags = new std::unordered_map<int64_t, std::map<std::string, std::string>>();
    m_uniform_buffer = EXPECT(Buffer::create(sizeof(FwChunkUniforms) * slice_count, WGPUBufferUsage_Uniform | WGPUBufferUsage_Vertex | WGPUBufferUsage_CopyDst));
}

Chunk::~Chunk()
{
    delete[] m_blocks;
    delete[] m_biomes;
    delete[] m_slices;
}

int Chunk::dimension() const
{
    return m_dim->id();
}

void Chunk::update_instance_buffer(glm::dvec3 position, uint32_t slice_index)
{
    glm::vec3 data((double)m_x * Chunk::width - position.x, (double)slice_index * Chunk::width - position.y, (double)m_z * Chunk::width - position.z);
    m_uniform_buffer->update_struct(data, slice_index * sizeof(glm::vec3));
}

void Chunk::set_block(int64_t x, int64_t y, int64_t z, BlockState state)
{
    if (y < 0 || y > Chunk::height)
        return;

    m_blocks[linearize(x, y, z)] = state;
    m_modified = true;

    m_dim->queue_rebuild(ChunkPos(m_x, m_z));

    if (x == 0)
        m_dim->queue_rebuild(ChunkPos(m_x - 1, m_z));
    if (x == 15)
        m_dim->queue_rebuild(ChunkPos(m_x + 1, m_z));
    if (z == 0)
        m_dim->queue_rebuild(ChunkPos(m_x, m_z - 1));
    if (z == 15)
        m_dim->queue_rebuild(ChunkPos(m_x, m_z + 1));

    // if (y == 0)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), 0, 1);
    // else if (y == 255)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), 15, 1);
    // else if (y % 16 == 0)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), y / 16 - 1, 2);
    // else if (y % 16 == 15)
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), y / 16, 2);
    // else
    //     m_dim->queue_rebuild(ChunkPos(m_x, m_z), y / 16, 1);
}

static uint64_t get_3d_seed(int64_t x, int64_t y, int64_t z, uint64_t custom_world_seed = 0)
{
    // Large 64-bit primes for bit distribution
    const uint64_t BIT_MUSH_1 = 0x9e3779b97f4a7c15ULL;
    const uint64_t BIT_MUSH_2 = 0xbf58476d1ce4e5b9ULL;
    const uint64_t BIT_MUSH_3 = 0x94d049bb133111ebULL;

    // Incorporate the coordinates and the global world seed
    uint64_t hash = custom_world_seed;
    hash ^= static_cast<uint64_t>(x) * BIT_MUSH_1;
    hash = (hash << 13) | (hash >> (64 - 13)); // Bitwise rotation

    hash ^= static_cast<uint64_t>(y) * BIT_MUSH_2;
    hash = (hash << 17) | (hash >> (64 - 17));

    hash ^= static_cast<uint64_t>(z) * BIT_MUSH_3;

    // Avalanche step to completely scramble the bits
    hash ^= hash >> 33;
    hash *= BIT_MUSH_2;
    hash ^= hash >> 29;
    hash *= BIT_MUSH_3;
    hash ^= hash >> 32;

    return hash; // Returns a pseudo-random 64-bit seed
}

static uint64_t xorshift(uint64_t seed)
{
    uint64_t x = seed;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x + 0x9E3779B97F4A7C15ULL;
}

std::expected<std::shared_ptr<Mesh>, Error> Chunk::build_opaque_mesh(size_t slice_index, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks)
{
    int64_t slice_y_offset = int64_t(slice_index) * width;
    MeshBuilder builder;

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (m_blocks[index].is_air())
                    continue;

                std::shared_ptr<Block> block = Engine::get().registry().get_block(m_blocks[index].id);
                if (block == nullptr)
                    return nullptr;

                auto match = [](BlockState *blocks, int64_t x, int64_t y, int64_t z, FaceKind face) -> bool
                {
                    BlockState state = blocks[linearize(x, y, z)];
                    if (state.is_air())
                        return false;
                    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
                    if (block != nullptr && !block->has_cullface(face))
                        return false;
                    return true; };
                auto match_cross_boundary = [](const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks, int64_t cx, int64_t cz, int64_t x, int64_t y, int64_t z, FaceKind face) -> bool
                {
                    auto iter = chunks.find(ChunkPos(cx, cz));
                    if (iter == chunks.end())
                        return false;
                    BlockState state = iter->second->get_block(x, y, z);
                    if (state.is_air())
                        return false;
                    std::shared_ptr<Block> block = Engine::get().registry().get_block(state.id);
                    if (block != nullptr && !block->has_cullface(face))
                        return false;
                    return true; };

                NeighborFlags flags{0};
                if ((x > 0 && match(m_blocks, x - 1, y, z, FaceKind::East)) || (x == 0 && match_cross_boundary(chunks, m_x - 1, m_z, 15, y, z, FaceKind::East)))
                    flags.value |= NeighborFlags::east;
                if ((x < 15 && match(m_blocks, x + 1, y, z, FaceKind::West)) || (x == 15 && match_cross_boundary(chunks, m_x + 1, m_z, 0, y, z, FaceKind::West)))
                    flags.value |= NeighborFlags::west;

                if (y > 0 && match(m_blocks, x, y - 1, z, FaceKind::Up))
                    flags.value |= NeighborFlags::up;
                if (y < height - 1 && match(m_blocks, x, y + 1, z, FaceKind::Down))
                    flags.value |= NeighborFlags::down;

                if ((z > 0 && match(m_blocks, x, y, z - 1, FaceKind::South)) || (z == 0 && match_cross_boundary(chunks, m_x, m_z - 1, x, y, 15, FaceKind::South)))
                    flags.value |= NeighborFlags::south;
                if ((z < 15 && match(m_blocks, x, y, z + 1, FaceKind::North)) || (z == 15 && match_cross_boundary(chunks, m_x, m_z + 1, x, y, 0, FaceKind::North)))
                    flags.value |= NeighborFlags::north;

                int64_t variant_index = 0;
                if (block->get_variant_count() > 1)
                {
                    uint64_t seed = get_3d_seed(x + m_x * 16, y, z + m_z * 16);
                    variant_index = int64_t(xorshift(seed) % block->get_variant_count());
                }
                block->add(builder, variant_index, {x, y - slice_y_offset, z}, flags);
            }
        }
    }

    if (builder.vertex_count() == 0)
        return nullptr;

    return builder.build();
}

static void add_water_mesh(MeshBuilder& builder, glm::i64vec3 position, NeighborFlags neighbors)
{
    static std::array<glm::vec3, 6> cube_normals{
        glm::vec3(0, 0, -1),
        glm::vec3(0, 0, 1),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(1, 0, 0),
    };

    const std::array<glm::vec3, 8> cube_vertices{
        (glm::vec3(-0.5, -0.5, -0.5)) + glm::vec3(position),
        (glm::vec3(+0.5, -0.5, -0.5)) + glm::vec3(position),
        (glm::vec3(+0.5, -0.5, +0.5)) + glm::vec3(position),
        (glm::vec3(-0.5, -0.5, +0.5)) + glm::vec3(position),

        (glm::vec3(-0.5, +0.5, -0.5)) + glm::vec3(position),
        (glm::vec3(+0.5, +0.5, -0.5)) + glm::vec3(position),
        (glm::vec3(+0.5, +0.5, +0.5)) + glm::vec3(position),
        (glm::vec3(-0.5, +0.5, +0.5)) + glm::vec3(position),
    };

    const struct
    {
        std::string name;
        FaceKind face;
        std::array<glm::vec3, 4> vertices;
    } faces[6]{
        {.name = "north", .face = FaceKind::North, .vertices = {cube_vertices[4], cube_vertices[5], cube_vertices[1], cube_vertices[0]}},
        {.name = "south", .face = FaceKind::South, .vertices = {cube_vertices[6], cube_vertices[7], cube_vertices[3], cube_vertices[2]}},
        {.name = "up", .face = FaceKind::Up, .vertices = {cube_vertices[4], cube_vertices[7], cube_vertices[6], cube_vertices[5]}},
        {.name = "down", .face = FaceKind::Down, .vertices = {cube_vertices[0], cube_vertices[1], cube_vertices[2], cube_vertices[3]}},
        {.name = "west", .face = FaceKind::West, .vertices = {cube_vertices[7], cube_vertices[4], cube_vertices[0], cube_vertices[3]}},
        {.name = "east", .face = FaceKind::East, .vertices = {cube_vertices[5], cube_vertices[6], cube_vertices[2], cube_vertices[1]}},
    };

    for (const auto& face : faces)
    {
        if (neighbors.has_opposite(face.face))
            continue;

        uint32_t i0 = builder.vertex_count() + 0;
        uint32_t i1 = builder.vertex_count() + 1;
        uint32_t i2 = builder.vertex_count() + 2;
        uint32_t i3 = builder.vertex_count() + 3;

        builder.add_index(i0);
        builder.add_index(i1);
        builder.add_index(i2);

        builder.add_index(i2);
        builder.add_index(i3);
        builder.add_index(i0);

        if (!neighbors.has(NeighborFlags::down))
        {
            builder.add_vertex(face.vertices[0] + glm::vec3(0, -0.2, 0));
            builder.add_vertex(face.vertices[1] + glm::vec3(0, -0.2, 0));
            builder.add_vertex(face.vertices[2] + glm::vec3(0, -0.2, 0));
            builder.add_vertex(face.vertices[3] + glm::vec3(0, -0.2, 0));
        }
        else
        {
            builder.add_vertex(face.vertices[0]);
            builder.add_vertex(face.vertices[1]);
            builder.add_vertex(face.vertices[2]);
            builder.add_vertex(face.vertices[3]);
        }

        builder.add_uv(glm::vec4(0.0, 0.0, 0, 0));
        builder.add_uv(glm::vec4(1.0, 0.0, 0, 0));
        builder.add_uv(glm::vec4(1.0, 1.0, 0, 0));
        builder.add_uv(glm::vec4(0.0, 1.0, 0, 0));

        const glm::vec3 normal = cube_normals[(int)face.face];
        builder.add_normal(normal);
        builder.add_normal(normal);
        builder.add_normal(normal);
        builder.add_normal(normal);
    }
}

std::expected<std::shared_ptr<Mesh>, Error> Chunk::build_water_mesh(size_t slice_index, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks)
{
    int64_t slice_y_offset = int64_t(slice_index) * width;
    MeshBuilder builder;

    for (int64_t x = 0; x < Chunk::width; x++)
    {
        for (int64_t y = slice_y_offset; y < slice_y_offset + Chunk::width; y++)
        {
            for (int64_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (!get_tag(index, "water").has_value())
                    continue;

                // TODO: add water gradient

                auto match = [](Chunk *chunk, int64_t x, int64_t y, int64_t z) -> bool
                {
                    if (chunk->get_tag({x, y, z}, "water").has_value())
                        return true;
                    return false; };
                auto match_cross_boundary = [](Chunk *chunk, const std::map<ChunkPos, std::shared_ptr<Chunk>>& chunks, int64_t cx, int64_t cz, int64_t x, int64_t y, int64_t z) -> bool
                {
                    auto iter = chunks.find(ChunkPos(cx, cz));
                    if (iter == chunks.end())
                        return true;
                    if (chunk->get_tag({x, y, z}, "water").has_value())
                        return true;
                    return false; };

                NeighborFlags flags{0};
                if ((x > 0 && match(this, x - 1, y, z)) || (x == 0 && match_cross_boundary(this, chunks, m_x - 1, m_z, 15, y, z)))
                    flags.value |= NeighborFlags::east;
                if ((x < 15 && match(this, x + 1, y, z)) || (x == 15 && match_cross_boundary(this, chunks, m_x + 1, m_z, 0, y, z)))
                    flags.value |= NeighborFlags::west;

                if (y > 0 && match(this, x, y - 1, z))
                    flags.value |= NeighborFlags::up;
                if (y < height - 1 && match(this, x, y + 1, z))
                    flags.value |= NeighborFlags::down;

                if ((z > 0 && match(this, x, y, z - 1)) || (z == 0 && match_cross_boundary(this, chunks, m_x, m_z - 1, x, y, 15)))
                    flags.value |= NeighborFlags::south;
                if ((z < 15 && match(this, x, y, z + 1)) || (z == 15 && match_cross_boundary(this, chunks, m_x, m_z + 1, x, y, 0)))
                    flags.value |= NeighborFlags::north;

                add_water_mesh(builder, {x, y - slice_y_offset, z}, flags);
            }
        }
    }

    if (builder.vertex_count() == 0)
        return nullptr;

    return builder.build();
}

void Chunk::set_tag(glm::i64vec3 pos, std::string_view name, Variant v, bool dont_modify)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    m_tags[key][std::string(name)] = v;
    if (!dont_modify)
        m_modified = true;
}

void Chunk::remove_tag(glm::i64vec3 pos, std::string_view name, bool dont_modify)
{
    uint16_t key = linearize(pos.x, pos.y, pos.z);
    auto tags = m_tags.find(key);

    if (tags != m_tags.end())
    {
        stdext::string_map<Variant>& block_tags = tags->second;
        block_tags.erase(block_tags.find(name));

        if (tags->second.size() == 0)
        {
            m_tags.erase(key);
            if (!dont_modify)
                m_modified = true;
        }
    }
}

std::optional<Variant> Chunk::get_tag(uint16_t index, std::string_view name) const
{
    auto tags = m_tags.find(index);
    if (tags != m_tags.end())
        return tags->second.find(name)->second;
    return std::nullopt;
}

std::optional<Variant> Chunk::get_tag(glm::i64vec3 pos, std::string_view name) const
{
    return get_tag(linearize(pos.x, pos.y, pos.z), name);
}

void Chunk::merge_tag(uint16_t index, const stdext::string_map<Variant>& tags, bool dont_modify)
{
    if (tags.size() == 0)
        return;

    for (const auto& [name, value] : tags)
        m_tags[index][name] = value;

    if (!dont_modify)
        m_modified = true;
}
