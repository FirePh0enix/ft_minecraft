#include "Resource/BlockState.hpp"

#include <nlohmann/json.hpp>

void from_json(const nlohmann::json& json, BlockStateResource& blockstate)
{
    json.at("variants").get_to(blockstate.variants);
}

void from_json(const nlohmann::json& json, BlockStateVariant& variant)
{
    json.at("model").get_to(variant.model);
}
