#include "World/Registry.hpp"

#include <filesystem>

#include <simdjson.h>

void BlockRegistry::load_blocks()
{
    for (auto& iter : std::filesystem::directory_iterator("assets/blocks"))
    {
        simdjson::ondemand::parser parser;
        simdjson::padded_string json_string = simdjson::padded_string::load(iter.path().string()).value_unsafe(); // TODO: Check errors
        simdjson::ondemand::document block = parser.iterate(json_string).value_unsafe();

        std::string_view name = block["name"].value_unsafe().get_string().value_unsafe();

        simdjson::ondemand::array faces_json = block["faces"].value_unsafe().get_array().value_unsafe();
        std::array<std::string, 6> faces;
        size_t i = 0;

        for (auto face : faces_json)
        {
            if (i > 5)
            {
                break;
            }

            faces[i] = face.value_unsafe().get_string().value_unsafe();
            i++;
        }

        GradientType gradient = GradientType::None;
        simdjson::simdjson_result<simdjson::ondemand::value> gradient_json = block["gradient"];

        if (gradient_json.error() == simdjson::SUCCESS && gradient_json.type().value_unsafe() == simdjson::ondemand::json_type::string)
        {
            std::string_view view = gradient_json.get_string().value_unsafe();
            if (view == "grass")
            {
                gradient = GradientType::Grass;
            }
            else if (view == "water")
            {
                gradient = GradientType::Water;
            }
        }

        info("Registering block `{}`", name);

        register_block(make_ref<Block>(std::string(name), faces, gradient));
    }
}
