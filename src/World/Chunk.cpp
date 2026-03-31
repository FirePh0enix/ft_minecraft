#include "World/Chunk.hpp"
#include "Profiler.hpp"
#include "World/Registry.hpp"
#include "World/World.hpp"

Chunk::Chunk(int64_t x, int64_t y, int64_t z, World *world)
    : m_x(x), m_y(y), m_z(z)
{
    // const glm::mat4 model_matrix = glm::translate(glm::identity<glm::mat4>(), glm::vec3(x * Chunk::width, y * Chunk::width, z * Chunk::width));

    // m_model.model_matrix = model_matrix;
    // m_model_buffer = RenderingDriver::get()->create_buffer(sizeof(Model), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest).value_or(nullptr);
    // m_model_buffer->update(View(m_model).as_bytes());

    // m_material = RenderingDriver::get()->create_material(world->m_shader, std::nullopt, MaterialFlagBits::Transparency, PolygonMode::Fill, CullMode::Back, UVType::UVT, "chunk");
    // m_material->set_param("images", BlockRegistry::get_texture_array());
    // m_material->set_param("env", world->m_env_buffer);
    // m_material->set_param("model", m_model_buffer);
}

Chunk::~Chunk()
{
}

void Chunk::set_block(int64_t x, int64_t y, int64_t z, BlockState state)
{
    m_blocks[linearize(x, y, z)] = state;
    // build_simple_mesh();
}

struct ChunkBlockFace
{
    uint8_t x;
    uint8_t y;
    uint8_t z;
    Axis axis;
    bool positive;
    uint32_t texture_index;

    ChunkBlockFace(uint8_t x,
                   uint8_t y,
                   uint8_t z,
                   Axis axis,
                   bool positive,
                   uint32_t texture_index) : x(x), y(y), z(z), axis(axis), positive(positive), texture_index(texture_index) {}
};

static std::array<glm::vec3, 4> vertex_from_axis(Axis axis, bool positive, glm::vec3 offset)
{
    glm::vec3 v[24]{
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z), // front 0
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),

        glm::vec3(0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z), // back 4
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),

        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z), // left 8
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),

        glm::vec3(0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z), // right 12
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),

        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z), // top 16
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, 0.5 + offset.y, -0.5 + offset.z),

        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z), // bottom 20
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, -0.5 + offset.z),
        glm::vec3(0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
        glm::vec3(-0.5 + offset.x, -0.5 + offset.y, 0.5 + offset.z),
    };

    if (axis == Axis::X && positive)
        return {v[12], v[13], v[14], v[15]};
    else if (axis == Axis::X)
        return {v[8], v[9], v[10], v[11]};
    else if (axis == Axis::Y && positive)
        return {v[16], v[17], v[18], v[19]};
    else if (axis == Axis::Y)
        return {v[20], v[21], v[22], v[23]};
    else if (axis == Axis::Z && positive)
        return {v[0], v[1], v[2], v[3]};
    else
        return {v[4], v[5], v[6], v[7]};
}

static glm::vec3 normal_from_axis(Axis axis, bool positive)
{
    if (axis == Axis::X)
        return glm::vec3(positive ? 1.0 : -1.0, 0.0, 0.0);
    else if (axis == Axis::Y)
        return glm::vec3(0.0, positive ? 1.0 : -1.0, 0.0);
    else if (axis == Axis::Z)
        return glm::vec3(0.0, 0.0, positive ? 1.0 : -1.0);
    return glm::vec3();
}

void Chunk::build_simple_mesh()
{
    ZoneScoped;

    // Let's detect which faces are not hidden.
    std::vector<ChunkBlockFace> faces;

    for (ssize_t x = 0; x < Chunk::width; x++)
    {
        for (ssize_t y = 0; y < Chunk::width; y++)
        {
            for (ssize_t z = 0; z < Chunk::width; z++)
            {
                const uint32_t index = linearize(x, y, z);

                if (m_blocks[index].is_air())
                    continue;

                const Ref<Block>& block = BlockRegistry::get_block_by_id(m_blocks[index].id);
                // println("{}", faces.size());

                if (m_blocks[linearize(x - 1, y, z)].is_air())
                    faces.push_back(ChunkBlockFace(x, y, z, Axis::X, false, block->get_texture_index(Axis::X, false)));
                if (m_blocks[linearize(x + 1, y, z)].is_air())
                    faces.push_back(ChunkBlockFace(x, y, z, Axis::X, true, block->get_texture_index(Axis::X, true)));

                if (m_blocks[linearize(x, y - 1, z)].is_air())
                    faces.push_back(ChunkBlockFace(x, y, z, Axis::Y, false, block->get_texture_index(Axis::Y, false)));
                if (m_blocks[linearize(x, y + 1, z)].is_air())
                    faces.push_back(ChunkBlockFace(x, y, z, Axis::Y, true, block->get_texture_index(Axis::Y, true)));

                if (m_blocks[linearize(x, y, z - 1)].is_air())
                    faces.push_back(ChunkBlockFace(x, y, z, Axis::Z, false, block->get_texture_index(Axis::Z, false)));
                if (m_blocks[linearize(x, y, z + 1)].is_air())
                    faces.push_back(ChunkBlockFace(x, y, z, Axis::Z, true, block->get_texture_index(Axis::Z, true)));
            }
        }
    }

    // No faces are visible, let's skip mesh generation.
    if (faces.empty())
    {
        m_empty = true;
        // m_mesh = nullptr;
        return;
    }

    m_empty = false;

    // Now we build a mesh from the faces.
    std::vector<uint16_t> indices;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec3> uvs;
    std::vector<glm::vec3> normals;

    for (const ChunkBlockFace& face : faces)
    {
        uint16_t i0 = vertices.size() + 0;
        uint16_t i1 = vertices.size() + 1;
        uint16_t i2 = vertices.size() + 2;
        uint16_t i3 = vertices.size() + 3;

        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        indices.push_back(i2);
        indices.push_back(i3);
        indices.push_back(i0);

        const std::array<glm::vec3, 4> new_vertices = vertex_from_axis(face.axis, face.positive, glm::vec3(face.x, face.y, face.z));
        vertices.push_back(new_vertices[0]);
        vertices.push_back(new_vertices[1]);
        vertices.push_back(new_vertices[2]);
        vertices.push_back(new_vertices[3]);

        uvs.push_back(glm::vec3(0.0, 0.0, (double)face.texture_index));
        uvs.push_back(glm::vec3(1.0, 0.0, (double)face.texture_index));
        uvs.push_back(glm::vec3(1.0, 1.0, (double)face.texture_index));
        uvs.push_back(glm::vec3(0.0, 1.0, (double)face.texture_index));

        const glm::vec3 normal = normal_from_axis(face.axis, face.positive);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
        normals.push_back(normal);
    }

    // m_mesh = Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), IndexType::Uint16, UVType::UVT);
}

void Chunk::build_tree()
{
    // Since chunks are 16x16x16, we need two levels of child nodes.
    //
    // Nodes are stored like this in memory:
    // | child0 | ... | childn | child0_child0 | ... | child0_childn | child1_child0 | ... | child1_childn
    //
    // Only `child_mask` is important here, other fields will be filled when building the top level buffer in the dimension.
    m_node.leaf = 0;
    m_node.child_ptr = 0;
    m_node.child_mask = 0;

    std::vector<SvtNode64> nodes2;
    m_nodes.push_back(SvtNode64{});

    for (uint32_t x = 0; x < 4; x++)
        for (uint32_t y = 0; y < 4; y++)
            for (uint32_t z = 0; z < 4; z++)
            {
                uint32_t index = z * 16 + y * 4 + x;

                SvtNode64 node{};
                node.leaf = 0;
                // Needs to be offset later by the number of "top-level" nodes.
                node.child_ptr = nodes2.size();
                node.child_mask = 0;

                for (uint32_t x2 = 0; x2 < 4; x2++)
                    for (uint32_t y2 = 0; y2 < 4; y2++)
                        for (uint32_t z2 = 0; z2 < 4; z2++)
                        {
                            uint32_t index2 = z2 * 16 + y2 * 4 + x2;
                            uint32_t index3 = (z * 4 * 4 * 4 * 4 * 4 + y * 4 * 4 * 4 * 4 + x * 4 * 4 * 4) + (z2 * 4 * 4 + y2 * 4 + x2);
                            BlockState state = m_blocks[index3];

                            if (state.is_air())
                                continue;

                            SvtNode64 node2{};
                            node2.leaf = 1;
                            node2.child_mask = 0;

                            // TODO: Since multiple blocks can have the same material, here we should have unique leaf data and reuse the index.
                            node2.child_ptr = m_leafs.size();
                            m_leafs.push_back(rand() % 5);

                            node.child_mask |= 1 << index2;

                            nodes2.push_back(node2);
                            m_empty = false;
                        }

                if (node.child_mask != 0)
                    m_node.child_mask |= 1 << index;
            }

    // now we offset the pointer by the number of "top-level" nodes.
    for (uint32_t i = 0; i < m_nodes.size(); i++)
        m_nodes[i].child_ptr += m_nodes.size() + 1;

    m_nodes[0] = m_node;
    m_nodes.insert(m_nodes.end(), nodes2.begin(), nodes2.end());
}
