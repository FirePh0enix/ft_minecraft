#include "World/Registry.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"

#include <filesystem>
#include <fstream>

#include <SDL3/SDL_surface.h>
#include <nlohmann/json.hpp>

struct BlockManifest
{
    enum class Model
    {
        Cube,
    };

    // TODO: replace `std::string` by `String`.

    std::string name;
    Model model;
    bool transparent = false;
    std::vector<std::string> faces;
    std::optional<GradientType> gradient;
};

void from_json(const nlohmann::json& j, String& m)
{
    const auto s = j.get_ptr<const nlohmann::json::string_t *>();
    m = String(s->data(), s->size());
}

void from_json(const nlohmann::json& j, BlockManifest& m)
{
    j.at("name").get_to(m.name);
    j.at("model").get_to(m.model);
    j.at("faces").get_to(m.faces);

    if (j.contains("gradient"))
        m.gradient = j.at("gradient");
    if (j.contains("transparent"))
        m.transparent = j.at("transparent");
}

NLOHMANN_JSON_SERIALIZE_ENUM(BlockManifest::Model, {
                                                       {BlockManifest::Model::Cube, "cube"},
                                                   });

NLOHMANN_JSON_SERIALIZE_ENUM(GradientType, {
                                               {GradientType::None, nullptr},
                                               {GradientType::Grass, "grass"},
                                               {GradientType::Water, "water"},
                                           });

BlockRegistry::BlockRegistry()
{
}

BlockRegistry::~BlockRegistry()
{
    for (auto& surface : m_textures)
        SDL_DestroySurface(surface);
}

Result<void> BlockRegistry::register_block(StringView name)
{
    String path = format("assets/blocks/{}.json", name);

    std::ifstream ifs(path.data(), std::ifstream::ate);
    String s;
    s.resize(ifs.tellg());
    ifs.seekg(0);
    ifs.read(s.data(), (std::streamsize)s.size());

    BlockManifest block_json = nlohmann::json::parse(std::string(s.data(), s.size()));
    std::array<String, 6> faces;

    // println("{} {}", name, block_json.transparent);

    for (size_t i = 0; i < faces.size(); i++)
    {
        faces[i] = StringView(block_json.faces[i]);
    }

    String names = name;

    std::array<String, 6> textures;
    for (size_t i = 0; i < 6; i++)
        textures[i].append(block_json.faces[i].data());

    const uint16_t id = m_blocks.size() + 1;
    Ref<Block> block = EXPECT(newref<Block>(names, id, textures, block_json.gradient.value_or(GradientType::None), block_json.transparent));

    (void)m_blocks.append(block);
    m_blocks_by_name[block->name()] = id;

    block->set_texture_ids({
        get_or_create(block->get_texture_names()[0]),
        get_or_create(block->get_texture_names()[1]),
        get_or_create(block->get_texture_names()[2]),
        get_or_create(block->get_texture_names()[3]),
        get_or_create(block->get_texture_names()[4]),
        get_or_create(block->get_texture_names()[5]),
    });

    return Result<void>();
}

void BlockRegistry::create_gpu_resources()
{
    uint32_t mip_level = 1;
    m_texture_array = EXPECT(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2DArray, m_textures.size(), mip_level));

    size_t index = 0;

    for (const auto& surface : m_textures)
    {
        m_texture_array->update(View((uint8_t *)surface->pixels, surface->w * surface->h * 4), index);

        Ref<Texture> texture = EXPECT(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2D));
        texture->update(View((uint8_t *)surface->pixels, surface->w * surface->h * 4));

        // TODO: create textureview instead of duplicating data in memory.
        EXPECT(m_texture_handles.append(texture));
        index++;
    }

    // s_texture_array->generate_mips();
}

uint32_t BlockRegistry::get_or_create(const StringView& name)
{
    const auto id_pair = m_texture_by_name.find(name);

    if (id_pair == m_texture_by_name.end())
    {
        String path = "assets/textures/";
        path.append(name);

        File file = EXPECT(Filesystem::open_file(path));
        LocalVector<char> buffer;
        EXPECT(file.reader().read_to_buffer(buffer));
        file.close();

        SDL_IOStream *texture_stream = SDL_IOFromConstMem(buffer.data(), buffer.size());

        SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);
        ERR_COND_V(texture_surface == nullptr, "Failed to parse image `{}`", path);

        // TODO: Check the format of the image and resize it if necessary.

        const uint32_t id = m_textures.size();

        (void)m_textures.append(texture_surface);
        m_texture_by_name[name] = id;

        // SDL_DestroySurface(texture_surface);
        SDL_CloseIO(texture_stream);

        return id;
    }
    else
    {
        return id_pair->second;
    }
}

Result<Ref<Entity>> EntityRegistry::create_entity(ClassHashCode class_hash)
{
    return m_entries.at(class_hash).c.value()();
}
