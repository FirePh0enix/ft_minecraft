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

#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

struct ModelObject
{
    std::string name;
    std::array<float, 3> size;
    std::array<float, 3> position;
    std::array<float, 3> origin;

    std::array<std::array<uint32_t, 2>, 24> uvs;
};

void from_json(const nlohmann::json& j, ModelObject& m)
{
    j.at("name").get_to(m.name);
    j.at("size").get_to(m.size);
    j.at("position").get_to(m.position);

    if (j.contains("origin"))
        j.at("origin").get_to(m.origin);

    if (j.contains("uvs"))
        j.at("uvs").get_to(m.uvs);
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

struct Keyframe
{
    uint32_t frame;
    std::vector<Transform> transforms;
};

void from_json(const nlohmann::json& j, Keyframe& m)
{
    j.at("frame").get_to(m.frame);
    j.at("transforms").get_to(m.transforms);
}

struct Animation
{
    std::string name;
    uint32_t fps;
    uint32_t frames;
    std::vector<Keyframe> keyframes;
};

void from_json(const nlohmann::json& j, Animation& m)
{
    j.at("name").get_to(m.name);
    j.at("fps").get_to(m.fps);
    j.at("frames").get_to(m.frames);
    j.at("keyframes").get_to(m.keyframes);
}

struct ModelJSON
{
    std::string name;
    std::array<float, 2> texture_size;
    std::string texture_path;
    std::vector<ModelObject> objects;
    std::vector<Animation> animations;
};

void from_json(const nlohmann::json& j, ModelJSON& m)
{
    j.at("name").get_to(m.name);
    j.at("objects").get_to(m.objects);
    j.at("animations").get_to(m.animations);

    if (j.contains("texture_size"))
        j.at("texture_size").get_to(m.texture_size);

    if (j.contains("texture_path"))
        j.at("texture_path").get_to(m.texture_path);
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

static Ref<Texture> load_texture(StringView path)
{
    Result<File> file = Filesystem::open_file(path);
    ERR_EXPECT_VR(file, 0, "Failed to open `{}`", path);

    const Vector<char>& buffer = file->read_to_buffer();
    SDL_IOStream *texture_stream = SDL_IOFromConstMem(buffer.data(), buffer.size());

    SDL_Surface *texture_surface = IMG_LoadPNG_IO(texture_stream);
    ERR_COND_V(texture_surface == nullptr, "Failed to parse image `{}`", path);

    auto texture = RenderingDriver::get()->create_texture(texture_surface->w, texture_surface->h, TextureFormat::RGBA8Srgb, TextureUsageFlagBits::CopyDest | TextureUsageFlagBits::Sampled, TextureDimension::D2D, 1, 1).value();
    texture->update(View((uint8_t *)texture_surface->pixels, texture_surface->w * texture_surface->h * 4));

    SDL_DestroySurface(texture_surface);
    SDL_CloseIO(texture_stream);

    return texture;
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
        shader->set_binding("global_model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 2, BindingAccess::Read));
        shader->set_binding("uvs", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 3, BindingAccess::Read));
        shader->set_binding("texture", Binding(BindingKind::Texture, ShaderStageFlagBits::Fragment, 0, 4, BindingAccess::Read, TextureDimension::D2D));

        shader->set_sampler("texture", SamplerDescriptor(Filter::Nearest, Filter::Nearest));
    }

    String source = TRY(Filesystem::open_file(path)).read_to_string();
    ModelJSON json = nlohmann::json::parse(std::string(source.data(), source.size()));

    Ref<Model> model = newref<Model>();
    model->m_name.append(json.name.data(), json.name.size());
    model->m_global_buffer = TRY(RenderingDriver::get()->create_buffer(sizeof(Model::Info), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));
    model->m_texture = load_texture(StringView(json.texture_path.data(), json.texture_path.size()));

    for (const auto& object : json.objects)
    {
        Model::Object obj;
        obj.name.append(object.name.data(), object.name.size());
        obj.model_buffer = TRY(RenderingDriver::get()->create_buffer(sizeof(Model::Info), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));
        obj.size = glm::vec3(object.size[0], object.size[1], object.size[2]);
        obj.position = glm::vec3(object.position[0], object.position[1], object.position[2]);
        obj.origin = glm::vec3(object.origin[0], object.origin[1], object.origin[2]);
        obj.material = RenderingDriver::get()->create_material(shader, std::nullopt, MaterialFlagBits::None, PolygonMode::Fill, CullMode::Back, UVType::UV);

        Vector<glm::vec2> uvs;
        for (uint32_t i = 0; i < 24; i++)
            TRY(uvs.append(glm::vec2(float(object.uvs[i][0]) / float(json.texture_size[0]), float(object.uvs[i][1]) / float(json.texture_size[1]))));
        obj.uv_buffer = TRY(RenderingDriver::get()->create_buffer_from_data(sizeof(glm::vec2) * 24, View(uvs).as_bytes(), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest));

        Model::Info info{
            // TODO: add rotation.
            .model_matrix = glm::translate(glm::identity<glm::mat4>(), obj.position) * glm::scale(glm::identity<glm::mat4>(), obj.size),
        };
        obj.model_buffer->update(View(info).as_bytes());

        obj.material->set_param("env", Engine::singleton->get_world()->get_env_buffer());
        obj.material->set_param("model", obj.model_buffer);
        obj.material->set_param("global_model", model->m_global_buffer);
        obj.material->set_param("uvs", obj.uv_buffer);
        obj.material->set_param("texture", model->m_texture);

        TRY(model->m_objects.append(obj));
    }

    for (const auto& animation : json.animations)
    {
        Model::Animation anim;
        anim.name.append(animation.name.data(), animation.name.size());
        anim.fps = animation.fps;
        anim.frames = animation.frames;

        for (const auto& action : animation.keyframes)
        {
            Model::Keyframe act;
            act.frame = action.frame;

            for (const auto& transform : action.transforms)
            {
                Model::Transform tran;
                tran.object_name.append(transform.object.data(), transform.object.size());
                tran.position = glm::vec3(transform.position[0], transform.position[1], transform.position[2]);
                tran.rotation = glm::vec3(transform.rotation[0], transform.rotation[1], transform.rotation[2]);

                TRY(act.transforms.append(tran));
            }

            TRY(anim.keyframes.append(act));
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
    m_frame = 0;
}

void Animator::play(String animation)
{
    if (m_animation_name == animation)
        return;

    m_animation_name = animation;
    m_time = 0.0;
    m_frame = 0;
}

void Animator::tick(float delta)
{
    std::optional<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
    {
        return;
    }

    Model::Animation animation = animation_maybe.value();

    update_model_animation_buffer();

    const float frame_time = 1.0f / float(animation.fps);
    m_time += delta;
    if (m_time >= frame_time)
    {
        m_frame = (m_frame + 1) % animation.frames;
        m_time -= frame_time;
    }
}

void Animator::update_model_animation_buffer()
{
    std::optional<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
    {
        return;
    }

    Model::Animation animation = animation_maybe.value();

    for (Model::Object& object : m_model->objects())
    {
        std::optional<TransformWithLength> current_twl_maybe = get_current_transform(object.name);
        std::optional<TransformWithLength> next_twl_maybe = get_next_transform(object.name);

        if (!current_twl_maybe.has_value() || !next_twl_maybe.has_value())
        {
            continue;
        }

        TransformWithLength current_twl = current_twl_maybe.value();
        TransformWithLength next_twl = next_twl_maybe.value();

        float t = 0.0;
        if (current_twl.frame_index < next_twl.frame_index)
        {
            t = float(m_frame - current_twl.frame_index) / float(next_twl.frame_index - current_twl.frame_index);
        }
        else
        {
            t = float(m_frame - current_twl.frame_index) / float(current_twl.frame_index - next_twl.frame_index);
        }

        const float x = std::lerp(current_twl.transform.rotation.x, next_twl.transform.rotation.x, t);
        const float y = std::lerp(current_twl.transform.rotation.y, next_twl.transform.rotation.y, t);
        const float z = std::lerp(current_twl.transform.rotation.z, next_twl.transform.rotation.z, t);

        const glm::mat4 scale_m = glm::scale(glm::identity<glm::mat4>(), object.size);
        const glm::mat4 translate_m = glm::translate(glm::identity<glm::mat4>(), object.position + object.origin);

        const glm::mat4 rotation_m = glm::rotate(glm::identity<glm::mat4>(), glm::radians(x), glm::vec3(1.0, 0.0, 0.0)) * glm::rotate(glm::identity<glm::mat4>(), glm::radians(y), glm::vec3(0.0, 1.0, 0.0)) * glm::rotate(glm::identity<glm::mat4>(), glm::radians(z), glm::vec3(0.0, 0.0, 1.0));
        const glm::mat4 origin_m = glm::translate(glm::identity<glm::mat4>(), -object.origin);
        Model::Info info{
            .model_matrix = translate_m * rotation_m * origin_m * scale_m,
        };
        object.model_buffer->update(View(info).as_bytes());
    }
}

std::optional<Model::Keyframe> Animator::get_keyframe_for_frame(uint32_t frame) const
{
    std::optional<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
        return std::nullopt;
    Model::Animation animation = animation_maybe.value();

    for (const auto& keyframe : animation.keyframes)
    {
        if (keyframe.frame == frame)
            return keyframe;
    }

    return std::nullopt;
}

std::optional<Animator::TransformWithLength> Animator::get_current_transform(String object_name) const
{
    std::optional<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
        return std::nullopt;
    Model::Animation animation = animation_maybe.value();

    uint32_t current_frame = m_frame;

    for (int32_t frame = int32_t(m_frame); frame >= 0; frame--)
    {
        std::optional<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
        if (!kf_maybe.has_value())
            continue;
        Model::Keyframe kf = kf_maybe.value();

        for (const auto& transform : kf.transforms)
        {
            if (transform.object_name == object_name)
                return Animator::TransformWithLength{.transform = transform, .frame_index = uint32_t(frame)};
        }
    }

    for (uint32_t frame = animation.frames - 1; frame > current_frame; frame--)
    {
        std::optional<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
        if (!kf_maybe.has_value())
            continue;
        Model::Keyframe kf = kf_maybe.value();

        for (const auto& transform : kf.transforms)
        {
            if (transform.object_name == object_name)
                return Animator::TransformWithLength{.transform = transform, .frame_index = uint32_t(frame)};
        }
    }

    return std::nullopt;
}

std::optional<Animator::TransformWithLength> Animator::get_next_transform(String object_name) const
{
    std::optional<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
        return std::nullopt;
    Model::Animation animation = animation_maybe.value();

    for (uint32_t frame = m_frame + 1; frame < animation.frames; frame++)
    {
        std::optional<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
        if (!kf_maybe.has_value())
            continue;
        Model::Keyframe kf = kf_maybe.value();

        for (const auto& transform : kf.transforms)
        {
            if (transform.object_name == object_name)
                return Animator::TransformWithLength{.transform = transform, .frame_index = uint32_t(frame)};
        }
    }

    for (uint32_t frame = 0; frame < animation.frames + 1; frame++)
    {
        std::optional<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
        if (!kf_maybe.has_value())
            continue;
        Model::Keyframe kf = kf_maybe.value();

        for (const auto& transform : kf.transforms)
        {
            if (transform.object_name == object_name)
                return Animator::TransformWithLength{.transform = transform, .frame_index = uint32_t(frame)};
        }
    }

    return std::nullopt;
}
