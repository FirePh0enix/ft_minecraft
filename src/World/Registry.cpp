#include "World/Registry.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Format.hpp"
#include "Core/Json.hpp"
#include "World/Block.hpp"

// #include <SDL3/SDL_surface.h>
#include <nlohmann/json.hpp>

struct BlockManifest
{
    enum class Model
    {
        Cube,
    };

    String name;
    Model model;
    bool transparent = false;
    Vector<String> faces;
    Option<GradientType> gradient;
};

void from_json(const nlohmann::json& j, BlockManifest& m)
{
    j.at("name").get_to(m.name);
    j.at("model").get_to(m.model);
    j.at("faces").get_to(m.faces);

    if (j.contains("gradient"))
        m.gradient = GradientType(j.at("gradient"));
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
        stbi_image_free((void *)surface.data);
}

Result<void> BlockRegistry::register_block(StringView name)
{
    String path = format("assets/blocks/{}.json", name);

    File file = TRY(Filesystem::open_file(path));
    String s = TRY(file.reader().read_to_string());

    BlockManifest block_json = nlohmann::json::parse(std::string(s.data(), s.size()));
    Array<String, 6> faces;

    // println("{} {}", name, block_json.transparent);

    for (size_t i = 0; i < faces.size(); i++)
    {
        faces[i] = StringView(block_json.faces.get_unchecked(i));
    }

    String names = name;

    Array<String, 6> textures;
    for (size_t i = 0; i < 6; i++)
        textures[i] = String(block_json.faces.get_unchecked(i).data());

    const uint16_t id = m_blocks.size() + 1;
    Ref<Block> block = EXPECT(newref<Block>(names, id, textures, block_json.gradient.value_or(GradientType::None), block_json.transparent));

    TRY(m_blocks.append(block));
    TRY(m_blocks_by_name.put(block->name(), id));

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
        m_texture_array->update(View((uint8_t *)surface.data, surface.w * surface.h * 4), index);

        Ref<Texture> texture = EXPECT(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2D));
        texture->update(View((uint8_t *)surface.data, surface.w * surface.h * 4));

        // TODO: create textureview instead of duplicating data in memory.
        EXPECT(m_texture_handles.append(texture));
        index++;
    }

    // s_texture_array->generate_mips();
}

uint32_t BlockRegistry::get_or_create(const StringView& name)
{
    const auto id_pair = m_texture_by_name.get(name);

    if (!id_pair.has_value())
    {
        String path = "assets/textures/";
        path.append(name);

        File file = EXPECT(Filesystem::open_file(path));
        LocalVector<char> buffer;
        EXPECT(file.reader().read_to_buffer(buffer));
        file.close();

        int w, h, channels;
        stbi_uc *data = stbi_load_from_memory((const stbi_uc *)buffer.data(), (int)buffer.size(), &w, &h, &channels, 4);
        ERR_COND_V(data == nullptr, "Failed to parse image `{}`", path);

        const uint32_t id = m_textures.size();

        EXPECT(m_textures.append(Image(data, w, h, channels)));
        EXPECT(m_texture_by_name.put(name, id));

        return id;
    }
    else
    {
        return id_pair.get();
    }
}

Result<Ref<Entity>> EntityRegistry::create_entity(ClassHashCode class_hash)
{
    return m_entries.get(class_hash).get().c();
}
