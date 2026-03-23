#include "World/Registry.hpp"
#include "SDL3/SDL_surface.h"
#include "nlohmann/json_fwd.hpp"

#include <filesystem>
#include <fstream>

#include <nlohmann/json.hpp>

struct BlockManifest
{
    enum class Model
    {
        Cube,
    };

    std::string name;
    Model model;
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
}

NLOHMANN_JSON_SERIALIZE_ENUM(BlockManifest::Model, {
                                                       {BlockManifest::Model::Cube, "cube"},
                                                   });

NLOHMANN_JSON_SERIALIZE_ENUM(GradientType, {
                                               {GradientType::None, nullptr},
                                               {GradientType::Grass, "grass"},
                                               {GradientType::Water, "water"},
                                           });

void BlockRegistry::load_blocks()
{
    // TODO: also change `std::filesystem::directory_iterator` to use the data pack system.

    for (auto& iter : std::filesystem::directory_iterator("assets/blocks"))
    {
        std::ifstream ifs(iter.path(), std::ifstream::ate);
        String s;
        s.resize(ifs.tellg());
        ifs.seekg(0);
        ifs.read(s.data(), (std::streamsize)s.size());

        BlockManifest block = nlohmann::json::parse(std::string(s.data(), s.size()));
        std::array<String, 6> faces;

        for (size_t i = 0; i < faces.size(); i++)
        {
            faces[i] = StringView(block.faces[i]);
        }

        info("Registering block `{}`", block.name);

        register_block(newobj(Block, StringView(block.name), faces, block.gradient.value_or(GradientType::None)));
    }
}

void BlockRegistry::register_block(Ref<Block> block)
{
    const uint16_t id = s_blocks.size() + 1;

    s_blocks.push_back(block);
    s_blocks_by_name[block->name()] = id;

    switch (block->get_variant())
    {
    case BlockStateVariant::Generic:
        s_generics.set(id);
        break;
    }

    block->set_texture_ids({
        get_or_create(block->get_texture_names()[0]),
        get_or_create(block->get_texture_names()[1]),
        get_or_create(block->get_texture_names()[2]),
        get_or_create(block->get_texture_names()[3]),
        get_or_create(block->get_texture_names()[4]),
        get_or_create(block->get_texture_names()[5]),
    });
}

void BlockRegistry::create_gpu_resources()
{
    auto texture_array_result = RenderingDriver::get()->create_texture(16, 16, TextureFormat::RGBA8Srgb, TextureUsageFlagBits::CopyDest | TextureUsageFlagBits::Sampled, TextureDimension::D2DArray, s_textures.size());
    ERR_COND_R(texture_array_result->is_null(), "Cannot create the block texture array");

    s_texture_array = texture_array_result.value();

    size_t index = 0;

    for (const auto& surface : s_textures)
    {
        s_texture_array->update(View((uint8_t *)surface->pixels, surface->w * surface->h * 4), index);
        index++;
    }

    s_texture_registry_buffer = RenderingDriver::get()->create_buffer(STRUCTNAME(glm::uvec4), s_blocks.size(), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Storage).value();
    std::vector<glm::uvec4> textures;

    for (const Ref<Block>& block : s_blocks)
    {
        std::array<uint32_t, 6> block_textures = block->get_texture_ids();
        textures.push_back(glm::uvec4(
            (block_textures[0] & 0xFF) | ((block_textures[1] & 0xFF) << 16),
            (block_textures[2] & 0xFF) | ((block_textures[3] & 0xFF) << 16),
            (block_textures[4] & 0xFF) | ((block_textures[5] & 0xFF) << 16),
            0));
    }

    s_texture_registry_buffer->update(View(textures).as_bytes());
}

void BlockRegistry::destroy()
{
    s_texture_array = nullptr;
    s_texture_registry_buffer = nullptr;

    s_texture_by_name.clear();
    for (auto& surface : s_textures)
        SDL_DestroySurface(surface);
    s_textures.clear();
}

uint32_t BlockRegistry::get_or_create(const StringView& name)
{
    const auto id_pair = s_texture_by_name.find(name);

    if (id_pair == s_texture_by_name.end())
    {
        String path = "assets/textures/";
        path.append(name);

        Result<File> file = Filesystem::open_file(path);
        ERR_EXPECT_VR(file, 0, "Failed to open `{}`", path);

        const std::vector<char>& buffer = file->read_to_buffer();
        SDL_IOStream *texture_stream = SDL_IOFromConstMem(buffer.data(), buffer.size());

        SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);
        ERR_COND_V(texture_surface == nullptr, "Failed to parse image `{}`", path);

        // TODO: Check the format of the image and resize it if necessary.

        const uint32_t id = s_textures.size();

        s_textures.push_back(texture_surface);
        s_texture_by_name[name] = id;

        // SDL_DestroySurface(texture_surface);
        SDL_CloseIO(texture_stream);

        return id;
    }
    else
    {
        return id_pair->second;
    }
}
