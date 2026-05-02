#include "World/Registry.hpp"
#include "SDL3/SDL_surface.h"
#include "nlohmann/json_fwd.hpp"
#include "webgpu/webgpu.h"

#include <filesystem>
#include <fstream>

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

        register_block(newobj(Block, StringView(block.name), faces, block.gradient.value_or(GradientType::None)));
    }
}

void BlockRegistry::register_block(Ref<Block> block)
{
    const uint16_t id = s_blocks.size() + 1;

    (void)s_blocks.append(block);
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
    uint32_t mip_level = 1;
    s_texture_array = EXPECT(Texture::create(16, 16, WGPUTextureFormat_RGBA8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding, TextureDimension::D2DArray, s_textures.size(), mip_level));

    size_t index = 0;

    for (const auto& surface : s_textures)
    {
        s_texture_array->update(View((uint8_t *)surface->pixels, surface->w * surface->h * 4), index);
        index++;
    }

    // s_texture_array->generate_mips();
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

        const Vector<char>& buffer = file->read_to_buffer();
        SDL_IOStream *texture_stream = SDL_IOFromConstMem(buffer.data(), buffer.size());

        SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);
        ERR_COND_V(texture_surface == nullptr, "Failed to parse image `{}`", path);

        // TODO: Check the format of the image and resize it if necessary.

        const uint32_t id = s_textures.size();

        (void)s_textures.append(texture_surface);
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
