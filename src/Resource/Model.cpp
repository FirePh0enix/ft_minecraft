#include "Resource/Model.hpp"

#include <nlohmann/json.hpp>

static bool is_ref(std::string_view path)
{
    return path.starts_with("#");
}

void Model::resolve(const Model& parent)
{
    elements.insert(elements.end(), parent.elements.begin(), parent.elements.end());
    textures.insert(parent.textures.begin(), parent.textures.end());

    resolve();

    // for (const auto& [name, ref] : textures)
    //     std::println("> {}: {}", name, ref);
}

void Model::resolve()
{
    for (auto& [name, ref] : textures)
    {
        if (ref.starts_with("#") && textures.contains(ref.substr(1))) // TODO: kinda janky, textures should probably be resolved at the end
            ref = textures[ref.substr(1)];
    }

    for (ModelElement& element : elements)
    {
        if (element.faces.contains("north") && is_ref(element.faces["north"].texture) && textures.contains(element.faces["north"].texture.substr(1)))
            element.faces["north"].texture = textures[element.faces.at("north").texture.substr(1)];
        if (element.faces.contains("south") && is_ref(element.faces["south"].texture) && textures.contains(element.faces["south"].texture.substr(1)))
            element.faces["south"].texture = textures[element.faces.at("south").texture.substr(1)];
        if (element.faces.contains("up") && is_ref(element.faces["up"].texture) && textures.contains(element.faces["up"].texture.substr(1)))
            element.faces["up"].texture = textures[element.faces.at("up").texture.substr(1)];
        if (element.faces.contains("down") && is_ref(element.faces["down"].texture) && textures.contains(element.faces["down"].texture.substr(1)))
            element.faces["down"].texture = textures[element.faces.at("down").texture.substr(1)];
        if (element.faces.contains("west") && is_ref(element.faces["west"].texture) && textures.contains(element.faces["west"].texture.substr(1)))
            element.faces["west"].texture = textures[element.faces.at("west").texture.substr(1)];
        if (element.faces.contains("east") && is_ref(element.faces["east"].texture) && textures.contains(element.faces["east"].texture.substr(1)))
            element.faces["east"].texture = textures[element.faces.at("east").texture.substr(1)];
    }
}

void from_json(const nlohmann::json& json, Model& model)
{
    if (json.contains("parent"))
        model.parent = json.at("parent");
    if (json.contains("ambientocclusion"))
        model.ambientocclusion = json.at("ambientocclusion");
    if (json.contains("textures"))
        model.textures = json.at("textures");
    if (json.contains("elements"))
        model.elements = json.at("elements");
}

void from_json(const nlohmann::json& json, ModelElement& element)
{
    if (json.contains("name"))
        element.name = json.at("name");
    json.at("from").get_to(element.from);
    json.at("to").get_to(element.to);
    json.at("faces").get_to(element.faces);
}

void from_json(const nlohmann::json& json, ModelFace& face)
{
    if (json.contains("uv"))
        face.uv = json.at("uv");
    json.at("texture").get_to(face.texture);
    if (json.contains("cullface"))
        face.cullface = json.at("cullface");
    if (json.contains("tintindex"))
        face.tintindex = json.at("tintindex");
}
