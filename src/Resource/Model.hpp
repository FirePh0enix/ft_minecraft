#pragma once

#include <array>
#include <optional>
#include <string>
#include <vector>

#include <nlohmann/json_fwd.hpp>

struct ModelFace
{
    std::array<int64_t, 4> uv;
    std::string texture;
    std::optional<std::string> cullface;
};

struct ModelElement
{
    std::string name;
    std::array<int64_t, 3> from{0, 0, 0};
    std::array<int64_t, 3> to{0, 0, 0};
    std::map<std::string, ModelFace> faces;
};

struct Model
{
    std::optional<std::string> parent;
    bool ambientocclusion;
    std::map<std::string, std::string> textures;
    std::vector<ModelElement> elements;

    void resolve(const Model& parent);
    void resolve();
};

void from_json(const nlohmann::json& json, Model& model);
void from_json(const nlohmann::json& json, ModelElement& model);
void from_json(const nlohmann::json& json, ModelFace& model);
