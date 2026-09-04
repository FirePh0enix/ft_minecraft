#include "Block/Block.hpp"

#include "Engine.hpp"
#include "World/Registry.hpp"

Block::Block(std::string_view path)
    : m_path(path)
{
    m_blockstate = EXPECT(Engine::get().registry().get_blockstate(path));
    m_model = EXPECT(Engine::get().registry().get_model(m_blockstate.variants[""][0].model));

    const struct
    {
        std::string name;
        FaceKind face;
    } faces[6]{
        {.name = "north", .face = FaceKind::North},
        {.name = "south", .face = FaceKind::South},
        {.name = "up", .face = FaceKind::Up},
        {.name = "down", .face = FaceKind::Down},
        {.name = "west", .face = FaceKind::West},
        {.name = "east", .face = FaceKind::East},
    };

    for (const auto& element : m_model.elements)
    {
        for (const auto& face : faces)
        {
            auto facei = element.faces.find(face.name);
            if (facei != element.faces.end() && facei->second.cullface.has_value()) // TODO: check value for unconventional face culling setup.
                m_cullfaces[(int)face.face] = true;
        }
    }

    // println("{} {} {} {} {} {}", m_cullfaces[0], m_cullfaces[1], m_cullfaces[2], m_cullfaces[3], m_cullfaces[4], m_cullfaces[5]);
}

bool Block::has_cullface(FaceKind face)
{
    return m_cullfaces[(int)face];
}

static std::array<glm::vec3, 6> cube_normals{
    glm::vec3(0, 0, -1),
    glm::vec3(0, 0, 1),
    glm::vec3(0, 1, 0),
    glm::vec3(0, -1, 0),
    glm::vec3(-1, 0, 0),
    glm::vec3(1, 0, 0),
};

// TODO: should be done when parsing the model json and not every time we create the mesh.
static FaceKind face_from_string(std::string_view name)
{
    if (name == "north")
        return FaceKind::North;
    if (name == "south")
        return FaceKind::South;
    if (name == "up")
        return FaceKind::Up;
    if (name == "down")
        return FaceKind::Down;
    if (name == "west")
        return FaceKind::West;
    if (name == "east")
        return FaceKind::East;
    return FaceKind::North;
}

void Block::add(glm::i64vec3 position, NeighborFlags neighbors, std::vector<uint32_t>& indices, std::vector<glm::vec3>& vertices, std::vector<glm::vec4>& uvs, std::vector<glm::vec3>& normals)
{
    for (const auto& element : m_model.elements)
    {
        const glm::vec3 offset = glm::vec3(element.from[0], element.from[1], element.from[2]) / 16.0f;
        const glm::vec3 size = glm::vec3(element.to[0] - element.from[0], element.to[1] - element.from[1], element.to[2] - element.from[1]) / 16.0f;

        const std::array<glm::vec3, 8> cube_vertices{
            (glm::vec3(-0.5, -0.5, -0.5) + offset) * size + glm::vec3(position),
            (glm::vec3(+0.5, -0.5, -0.5) + offset) * size + glm::vec3(position),
            (glm::vec3(+0.5, -0.5, +0.5) + offset) * size + glm::vec3(position),
            (glm::vec3(-0.5, -0.5, +0.5) + offset) * size + glm::vec3(position),

            (glm::vec3(-0.5, +0.5, -0.5) + offset) * size + glm::vec3(position),
            (glm::vec3(+0.5, +0.5, -0.5) + offset) * size + glm::vec3(position),
            (glm::vec3(+0.5, +0.5, +0.5) + offset) * size + glm::vec3(position),
            (glm::vec3(-0.5, +0.5, +0.5) + offset) * size + glm::vec3(position),
        };

        const struct
        {
            std::string name;
            FaceKind face;
            std::array<glm::vec3, 4> vertices;
        } faces[6]{
            {.name = "north", .face = FaceKind::North, .vertices = {cube_vertices[0], cube_vertices[4], cube_vertices[5], cube_vertices[1]}},
            {.name = "south", .face = FaceKind::South, .vertices = {cube_vertices[6], cube_vertices[7], cube_vertices[3], cube_vertices[2]}},
            {.name = "up", .face = FaceKind::Up, .vertices = {cube_vertices[4], cube_vertices[7], cube_vertices[6], cube_vertices[5]}},
            {.name = "down", .face = FaceKind::Down, .vertices = {cube_vertices[0], cube_vertices[1], cube_vertices[2], cube_vertices[3]}},
            {.name = "west", .face = FaceKind::West, .vertices = {cube_vertices[7], cube_vertices[4], cube_vertices[0], cube_vertices[3]}},
            {.name = "east", .face = FaceKind::East, .vertices = {cube_vertices[5], cube_vertices[6], cube_vertices[2], cube_vertices[1]}},
        };

        for (const auto& face : faces)
        {
            auto facei = element.faces.find(face.name);
            if (facei != element.faces.end())
            {
                if (facei->second.cullface.has_value() && neighbors.has_opposite(face_from_string(facei->second.cullface.value())))
                    continue;

                uint32_t i0 = vertices.size() + 0;
                uint32_t i1 = vertices.size() + 1;
                uint32_t i2 = vertices.size() + 2;
                uint32_t i3 = vertices.size() + 3;

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);

                indices.push_back(i2);
                indices.push_back(i3);
                indices.push_back(i0);

                vertices.push_back(face.vertices[0]);
                vertices.push_back(face.vertices[1]);
                vertices.push_back(face.vertices[2]);
                vertices.push_back(face.vertices[3]);

                const AtlasTextureData& data = Engine::get().registry().get_atlas_data(facei->second.texture);
                const float x_min = float(data.x) / Engine::get().registry().get_atlas_size();
                const float x_max = float(data.x + data.width) / Engine::get().registry().get_atlas_size();
                const float y_min = float(data.y) / Engine::get().registry().get_atlas_size();
                const float y_max = float(data.y + data.height) / Engine::get().registry().get_atlas_size();

                uvs.push_back(glm::vec4(x_min, y_min, 0, 0));
                uvs.push_back(glm::vec4(x_max, y_min, 0, 0));
                uvs.push_back(glm::vec4(x_max, y_max, 0, 0));
                uvs.push_back(glm::vec4(x_min, y_max, 0, 0));

                const glm::vec3 normal = cube_normals[(int)face.face];
                normals.push_back(normal);
                normals.push_back(normal);
                normals.push_back(normal);
                normals.push_back(normal);
            }
        }
    }
}
