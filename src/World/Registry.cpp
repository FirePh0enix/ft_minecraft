#include "World/Registry.hpp"

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
        std::string s;
        s.resize(ifs.tellg());
        ifs.seekg(0);
        ifs.read(s.data(), (std::streamsize)s.size());

        BlockManifest block = nlohmann::json::parse(s.data());
        std::array<std::string, 6> faces;

        for (size_t i = 0; i < faces.size(); i++)
            faces[i] = block.faces[i];

        info("Registering block `{}`", block.name);

        register_block(make_ref<Block>(block.name, faces, block.gradient.value_or(GradientType::None)));
    }
}
