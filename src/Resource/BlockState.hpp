#pragma once

#include <map>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

struct BlockStateVariant
{
    std::string model;
};

struct BlockStateResource
{
    /// Key are conditions for the variant to materialize, except an empty key which means it will always be selected.
    std::map<std::string, std::vector<BlockStateVariant>> variants;
};

void from_json(const nlohmann::json& json, BlockStateResource& blockstate);
void from_json(const nlohmann::json& json, BlockStateVariant& variant);
