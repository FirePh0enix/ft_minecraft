#include "Model.hpp"

#include "Core/Filesystem.hpp"
#include "Core/Ref.hpp"
#include "Engine.hpp"
#include "Render/Driver.hpp"
#include "Render/Types.hpp"
#include "glm/ext/matrix_float4x4.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/trigonometric.hpp"

#include <cmath>
#include <nlohmann/json.hpp>
#include <optional>

struct ModelObject
{
    std::string name;
    std::array<float, 3> size;
    std::array<float, 3> position;
};

void from_json(const nlohmann::json& j, ModelObject& m)
{
    j.at("name").get_to(m.name);
    j.at("size").get_to(m.size);
    j.at("position").get_to(m.position);
}

struct Transform
{
    std::string object;
    std::array<float, 3> position;
    std::array<float, 3> rotation;
};

void from_json(const nlohmann::json& j, Transform& m)
{
    j.at("object").get_to(m.object);
    if (j.contains("position"))
        j.at("position").get_to(m.position);
    if (j.contains("rotation"))
        j.at("rotation").get_to(m.rotation);
}

struct Action
{
    float time;
    std::vector<Transform> transforms;
};

void from_json(const nlohmann::json& j, Action& m)
{
    j.at("time").get_to(m.time);
    j.at("transforms").get_to(m.transforms);
}

struct Animation
{
    std::string name;
    std::vector<Action> actions;
};

void from_json(const nlohmann::json& j, Animation& m)
{
    j.at("name").get_to(m.name);
    j.at("actions").get_to(m.actions);
}

struct ModelJSON
{
    std::string name;
    std::vector<ModelObject> objects;
    std::vector<Animation> animations;
};

void from_json(const nlohmann::json& j, ModelJSON& m)
{
    j.at("name").get_to(m.name);
    j.at("objects").get_to(m.objects);
    j.at("animations").get_to(m.animations);
}

static Result<Ref<Mesh>> create_cube_mesh(glm::vec3 size = glm::vec3(1.0), glm::vec3 offset = glm::vec3())
{
    const glm::vec3 hs = size / glm::vec3(2.0);

    // clang-format off
    std::array<uint16_t, 36> indices{
        0, 1, 2,
        2, 3, 0, // front

        20, 21, 22,
        22, 23, 20, // back

        4, 5, 6,
        6, 7, 4, // right

        12, 13, 14,
        14, 15, 12, // left

        8, 9, 10,
        10, 11, 8, // top

        16, 17, 18,
        18, 19, 16, // bottom
    };
    // clang-format on

    std::array<glm::vec3, 24> vertices{
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // front
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // back
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // left
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z), // right
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, hs.y + offset.y, hs.z + offset.z), // top
        glm::vec3(hs.x + offset.x, hs.y + offset.y, hs.z + offset.z),
        glm::vec3(hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, hs.y + offset.y, -hs.z + offset.z),

        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z), // bottom
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, -hs.z + offset.z),
        glm::vec3(hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
        glm::vec3(-hs.x + offset.x, -hs.y + offset.y, hs.z + offset.z),
    };

    std::array<glm::vec2, 24> uvs{
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),

        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };

    std::array<glm::vec3, 24> normals{
        glm::vec3(0.0, 0.0, 1.0), // front
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),
        glm::vec3(0.0, 0.0, 1.0),

        glm::vec3(0.0, 0.0, -1.0), // back
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),
        glm::vec3(0.0, 0.0, -1.0),

        glm::vec3(1.0, 0.0, 0.0), // left
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),
        glm::vec3(1.0, 0.0, 0.0),

        glm::vec3(-1.0, 0.0, 0.0), // right
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),
        glm::vec3(-1.0, 0.0, 0.0),

        glm::vec3(0.0, 1.0, 0.0), // top
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),
        glm::vec3(0.0, 1.0, 0.0),

        glm::vec3(0.0, -1.0, 0.0), // bottom
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
        glm::vec3(0.0, -1.0, 0.0),
    };

    return Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), IndexType::Uint16);
}

Result<Ref<Model>> Model::load(const StringView& path)
{
    if (mesh.is_null())
    {
        mesh = TRY(create_cube_mesh());
    }
    if (shader.is_null())
    {
        shader = Shader::load("assets/shaders/model.wgsl").value();
        shader->set_binding("env", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 0, BindingAccess::Read));
        shader->set_binding("model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 1, BindingAccess::Read));
        shader->set_binding("animation_model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 2, BindingAccess::Read));
        shader->set_binding("global_model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 3, BindingAccess::Read));
    }

    String source = TRY(Filesystem::open_file(path)).read_to_string();
    ModelJSON json = nlohmann::json::parse(std::string(source.data(), source.size()));

    Ref<Model> model = newref<Model>();
    model->m_name.append(json.name.data(), json.name.size());
    model->m_global_buffer = TRY(RenderingDriver::get()->create_buffer(sizeof(Model::Info), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));

    for (const auto& object : json.objects)
    {
        Model::Object obj;
        obj.name.append(object.name.data(), object.name.size());
        obj.model_buffer = TRY(RenderingDriver::get()->create_buffer(sizeof(Model::Info), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));
        obj.animation_buffer = TRY(RenderingDriver::get()->create_buffer(sizeof(Model::Info), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));
        obj.size = glm::vec3(object.size[0], object.size[1], object.size[2]);
        obj.position = glm::vec3(object.position[0], object.position[1], object.position[2]);
        obj.material = RenderingDriver::get()->create_material(shader, std::nullopt, MaterialFlagBits::None, PolygonMode::Fill, CullMode::Back, UVType::UV);

        Model::Info info{
            // TODO: add rotation.
            .model_matrix = glm::translate(glm::identity<glm::mat4>(), obj.position) * glm::scale(glm::identity<glm::mat4>(), obj.size),
        };
        obj.model_buffer->update(View(info).as_bytes());

        info.model_matrix = glm::identity<glm::mat4>();
        obj.animation_buffer->update(View(info.model_matrix).as_bytes());

        obj.material->set_param("env", Engine::singleton->get_world()->get_env_buffer());
        obj.material->set_param("model", obj.model_buffer);
        obj.material->set_param("animation_model", obj.animation_buffer);
        obj.material->set_param("global_model", model->m_global_buffer);

        TRY(model->m_objects.append(obj));
    }

    for (const auto& animation : json.animations)
    {
        Model::Animation anim;
        anim.name.append(animation.name.data(), animation.name.size());

        for (const auto& action : animation.actions)
        {
            Model::Action act;
            act.time = action.time;

            for (const auto& transform : action.transforms)
            {
                Model::Transform tran;
                tran.object_name.append(transform.object.data(), transform.object.size());
                tran.position = glm::vec3(transform.position[0], transform.position[1], transform.position[2]);
                tran.rotation = glm::vec3(transform.rotation[0], transform.rotation[1], transform.rotation[2]);

                TRY(act.transforms.append(tran));
            }

            TRY(anim.actions.append(act));
        }

        TRY(model->m_animation.append(anim));
    }

    return model;
}

void Animator::set_model(Ref<Model> model)
{
    m_model = model;
    m_animation_name = "";
    m_time = 0.0;
    m_animation_index = 0;
}

void Animator::play(String animation)
{
    if (m_animation_name == animation)
        return;

    m_animation_name = animation;
    m_time = 0.0;
    m_animation_index = 0;

    for (const auto& object : m_model->objects())
    {
        m_previous_transforms[std::string(object.name.data())] = Model::Transform();
    }
}

void Animator::tick(float delta)
{
    Model::Animation animation = m_model->get_animation(m_animation_name).value(); // TODO: check errors

    if (m_animation_index >= animation.actions.size())
    {
        // loop the animation
        m_animation_index = 0;
        m_time = 0;
    }

    const Model::Action& action = animation.actions.get_unchecked(m_animation_index);

    for (const auto& transform : action.transforms)
    {
        std::optional<Model::Object> obj_maybe = m_model->get_object(transform.object_name);
        if (!obj_maybe.has_value())
            continue;

        Model::Object obj = obj_maybe.value();

        const Model::Transform& previous_transform = m_previous_transforms[std::string(obj.name.data())];
        float x = std::lerp(previous_transform.rotation.x, transform.rotation.x, m_time / action.time);
        float y = std::lerp(previous_transform.rotation.y, transform.rotation.y, m_time / action.time);
        float z = std::lerp(previous_transform.rotation.z, transform.rotation.z, m_time / action.time);

        const glm::mat4 rotation_m = glm::rotate(glm::identity<glm::mat4>(), glm::radians(x), glm::vec3(1.0, 0.0, 0.0)) * glm::rotate(glm::identity<glm::mat4>(), glm::radians(y), glm::vec3(0.0, 1.0, 0.0)) * glm::rotate(glm::identity<glm::mat4>(), glm::radians(z), glm::vec3(0.0, 0.0, 1.0));
        const glm::mat4 translate_m = glm::translate(glm::identity<glm::mat4>(), transform.position);

        Model::Info info{
            .model_matrix = rotation_m * translate_m,
        };
        obj.animation_buffer->update(View(info).as_bytes());
    }

    if (m_time >= action.time)
    {
        m_time -= action.time;
        m_animation_index += 1;
    }
    else
    {
        m_time += delta;
    }
}
