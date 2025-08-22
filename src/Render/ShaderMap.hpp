#pragma once

#include <nlohmann/json.hpp>

#include "Render/Types.hpp"

NLOHMANN_JSON_SERIALIZE_ENUM(BindingKind, {
                                              {BindingKind::Texture, "texture"},
                                              {BindingKind::UniformBuffer, "uniformBuffer"},
                                              {BindingKind::PushConstants, "pushConstants"},
                                          });

NLOHMANN_JSON_SERIALIZE_ENUM(ShaderStageFlagBits, {
                                                      {ShaderStageFlagBits::Vertex, "vertex"},
                                                      {ShaderStageFlagBits::Fragment, "fragment"},
                                                      {ShaderStageFlagBits::Compute, "compute"},
                                                  });

enum class ShaderVariant
{
    Base = 0,
    DepthPass = 1,
};

NLOHMANN_JSON_SERIALIZE_ENUM(ShaderVariant, {
                                                {ShaderVariant::Base, "base"},
                                                {ShaderVariant::DepthPass, "depth_pass"},
                                            });

static inline const char *variants[] = {"", "DEPTH_PASS"};

struct ShaderMap
{
    struct Uniform
    {
        std::string name;
        BindingKind type;
        uint32_t set;
        uint32_t binding;
        ShaderStageFlags stage;
    };

    struct Stage
    {
        ShaderStageFlagBits stage;
        std::string file;
    };

    struct Variant
    {
        ShaderVariant variant;
        std::vector<Stage> stages;
    };

    std::vector<Variant> variants;
    std::vector<Uniform> uniforms;
};

inline void to_json(nlohmann::json& j, const ShaderStageFlags& stage_mask)
{
    // Encode only one mask, i think Vulkan support only one anyway but this could still be a problem.
    // TODO: Maybe deny multiple stages during compilation ?

    if (stage_mask.has_any(ShaderStageFlagBits::Vertex))
        j = "vertex";
    else if (stage_mask.has_any(ShaderStageFlagBits::Fragment))
        j = "fragment";
    else if (stage_mask.has_any(ShaderStageFlagBits::Compute))
        j = "compute";
    else
        j = nullptr;
}

inline void from_json(const nlohmann::json& j, ShaderStageFlags& stage_mask)
{
    if (j == "vertex")
        stage_mask = ShaderStageFlagBits::Vertex;
    else if (j == "fragment")
        stage_mask = ShaderStageFlagBits::Fragment;
    else if (j == "compute")
        stage_mask = ShaderStageFlagBits::Compute;
    else
        stage_mask = ShaderStageFlagBits{};
}

inline void to_json(nlohmann::json& j, const ShaderMap::Uniform& uniform)
{
    j = nlohmann::json{{"name", uniform.name}, {"type", uniform.type}, {"set", uniform.set}, {"binding", uniform.binding}, {"stage", uniform.stage}};
}

inline void from_json(const nlohmann::json& j, ShaderMap::Uniform& uniform)
{
    j.at("name").get_to(uniform.name);
    j.at("type").get_to(uniform.type);
    j.at("set").get_to(uniform.set);
    j.at("binding").get_to(uniform.binding);
    j.at("stage").get_to(uniform.stage);
}

inline void to_json(nlohmann::json& j, const ShaderMap::Stage& stage)
{
    j = nlohmann::json{{"stage", stage.stage}, {"file", stage.file}};
}

inline void from_json(const nlohmann::json& j, ShaderMap::Stage& stage)
{
    j.at("stage").get_to(stage.stage);
    j.at("file").get_to(stage.file);
}

inline void to_json(nlohmann::json& j, const ShaderMap::Variant& variant)
{
    j = nlohmann::json{{"stages", variant.stages}, {"variant", variant.variant}};
}

inline void from_json(const nlohmann::json& j, ShaderMap::Variant& variant)
{
    j.at("variant").get_to(variant.variant);
    j.at("stages").get_to(variant.stages);
}

inline void to_json(nlohmann::json& j, const ShaderMap& map)
{
    j = nlohmann::json{{"variants", map.variants}, {"uniforms", map.uniforms}};
}

inline void from_json(const nlohmann::json& j, ShaderMap& map)
{
    j.at("variants").get_to(map.variants);
    j.at("uniforms").get_to(map.uniforms);
}
