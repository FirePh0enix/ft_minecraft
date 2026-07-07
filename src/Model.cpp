#include "Model.hpp"

#include "Core/Containers/Array.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Json.hpp"
#include "Core/Ref.hpp"
#include "Core/String.hpp"
#include "Render/Renderer.hpp"
#include "Render/Types.hpp"

#include <cmath>

#include <SDL3/SDL.h>
#include <nlohmann/json.hpp>
#include <stb_image.h>

struct ModelObject
{
    String name;
    Array<float, 3> size;
    Array<float, 3> position;
    Array<float, 3> origin;

    Array<Array<uint32_t, 2>, 24> uvs;
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
    String object;
    Array<float, 3> position;
    Array<float, 3> rotation;
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
    Vector<Transform> transforms;
};

void from_json(const nlohmann::json& j, Keyframe& m)
{
    j.at("frame").get_to(m.frame);
    j.at("transforms").get_to(m.transforms);
}

struct Animation
{
    String name;
    uint32_t fps;
    uint32_t frames;
    Vector<Keyframe> keyframes;
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
    String name;
    Array<float, 2> texture_size;
    String texture_path;
    Vector<ModelObject> objects;
    Vector<Animation> animations;
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

Result<Ref<Model>> Model::load(const StringView& path)
{
    File file = TRY(Filesystem::open_file(path));
    String source = TRY(file.reader().read_to_string());

    ModelJSON json = nlohmann::json::parse(std::string(source.data(), source.size()));

    Ref<Model> model = newref<Model>();
    model->m_name.append(json.name.data(), json.name.size());
    model->m_global_buffer = TRY(Buffer::create(sizeof(Model::Info), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
    model->m_texture = TRY(Texture::load(json.texture_path));

    for (const auto& object : json.objects)
    {
        Model::Object obj;
        obj.name.append(object.name.data(), object.name.size());
        obj.model_buffer = TRY(Buffer::create(sizeof(Model::Info), WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
        obj.size = glm::vec3(object.size[0], object.size[1], object.size[2]);
        obj.position = glm::vec3(object.position[0], object.position[1], object.position[2]);
        obj.origin = glm::vec3(object.origin[0], object.origin[1], object.origin[2]);
        obj.bg = BindGroup::create(Renderer::get().get_fw_model_shader());

        Vector<glm::vec2> uvs;
        for (uint32_t i = 0; i < 24; i++)
            uvs.append(glm::vec2(float(object.uvs[i][0]) / float(json.texture_size[0]), float(object.uvs[i][1]) / float(json.texture_size[1])));
        obj.uv_buffer = TRY(Buffer::create(sizeof(glm::vec2) * 24, WGPUBufferUsage_Uniform | WGPUBufferUsage_CopyDst));
        obj.uv_buffer->update(View(uvs).as_bytes());

        Model::Info info{
            // TODO: add rotation.
            .model_matrix = glm::rotate(glm::identity<glm::mat4>(), float(M_PI), glm::vec3(0, 1, 0)) * glm::translate(glm::identity<glm::mat4>(), obj.position) * glm::scale(glm::identity<glm::mat4>(), obj.size),
        };
        obj.model_buffer->update(View(info).as_bytes());

        obj.bg->set_param("camera", Renderer::get().get_fw_camera());
        obj.bg->set_param("model", obj.model_buffer);
        obj.bg->set_param("global_model", model->m_global_buffer);
        obj.bg->set_param("uvs", obj.uv_buffer);
        obj.bg->set_param("texture", model->m_texture);

        model->m_objects.append(obj);
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

                act.transforms.append(tran);
            }

            anim.keyframes.append(act);
        }

        model->m_animation.append(anim);
    }

    return model;
}

View<Model::Object> Model::objects() const
{
    return m_objects;
}

Ref<Buffer> Model::get_global_buffer() const
{
    return m_global_buffer;
}

Option<Model::Object> Model::get_object(String name) const
{
    for (const auto& obj : m_objects)
    {
        if (obj.name == name)
            return obj;
    }
    return None;
}

void Model::encode(const RenderPass& pass, const Transform3D& transform)
{
    const Ref<Mesh>& mesh = Renderer::get().get_cube_mesh();

    Info info{.model_matrix = transform.to_matrix()};
    m_global_buffer->update(View(info).as_bytes());

    for (const auto& obj : m_objects)
        Renderer::get().draw(pass, mesh, Renderer::get().get_fw_model_mat(), obj.bg);
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
    Option<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
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
    Option<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
    {
        return;
    }

    Model::Animation animation = animation_maybe.value();

    for (Model::Object& object : m_model->objects())
    {
        Option<TransformWithLength> current_twl_maybe = get_current_transform(object.name);
        Option<TransformWithLength> next_twl_maybe = get_next_transform(object.name);

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

Option<Model::Keyframe> Animator::get_keyframe_for_frame(uint32_t frame) const
{
    Option<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
        return None;
    Model::Animation animation = animation_maybe.value();

    for (const auto& keyframe : animation.keyframes)
    {
        if (keyframe.frame == frame)
            return keyframe;
    }

    return None;
}

Option<Animator::TransformWithLength> Animator::get_current_transform(String object_name) const
{
    Option<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
        return None;
    Model::Animation animation = animation_maybe.value();

    uint32_t current_frame = m_frame;

    for (int32_t frame = int32_t(m_frame); frame >= 0; frame--)
    {
        Option<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
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
        Option<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
        if (!kf_maybe.has_value())
            continue;
        Model::Keyframe kf = kf_maybe.value();

        for (const auto& transform : kf.transforms)
        {
            if (transform.object_name == object_name)
                return Animator::TransformWithLength{.transform = transform, .frame_index = uint32_t(frame)};
        }
    }

    return None;
}

Option<Animator::TransformWithLength> Animator::get_next_transform(String object_name) const
{
    Option<Model::Animation> animation_maybe = m_model->get_animation(m_animation_name);
    if (!animation_maybe.has_value())
        return None;
    Model::Animation animation = animation_maybe.value();

    for (uint32_t frame = m_frame + 1; frame < animation.frames; frame++)
    {
        Option<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
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
        Option<Model::Keyframe> kf_maybe = get_keyframe_for_frame(frame);
        if (!kf_maybe.has_value())
            continue;
        Model::Keyframe kf = kf_maybe.value();

        for (const auto& transform : kf.transforms)
        {
            if (transform.object_name == object_name)
                return Animator::TransformWithLength{.transform = transform, .frame_index = uint32_t(frame)};
        }
    }

    return None;
}
