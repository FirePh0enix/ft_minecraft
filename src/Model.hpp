#pragma once

#include "Core/Containers/Vector.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Ref.hpp"
#include "Core/String.hpp"
#include "Render/Driver.hpp"

class Model : public Object
{
    CLASS(Model, Object);

public:
    struct Info
    {
        glm::mat4 model_matrix;
    };

    struct Object
    {
        String name;

        glm::vec3 size;
        glm::vec3 position;
        glm::vec3 origin;

        Ref<Buffer> model_buffer;
        Ref<Buffer> uv_buffer;

        Ref<Material> material;
    };

    struct Transform
    {
        String object_name;
        glm::vec3 position;
        glm::vec3 rotation;
    };

    struct Keyframe
    {
        uint32_t frame;
        Vector<Transform> transforms;
    };

    struct Animation
    {
        String name;
        uint32_t fps;
        uint32_t frames;
        Vector<Keyframe> keyframes;
    };

    static Result<Ref<Model>> load(const StringView& path);

    String name() const { return m_name; }
    View<Object> objects() const { return m_objects; }
    Ref<Buffer> get_global_buffer() const { return m_global_buffer; }

    // TODO: this sould be handled in the new renderer.
    void set_texture(Ref<Texture> texture);

    std::optional<Animation> get_animation(String name) const
    {
        for (const Animation& anim : m_animation)
        {
            if (anim.name == name)
                return anim;
        }
        return std::nullopt;
    }

    std::optional<Object> get_object(String name) const
    {
        for (const auto& obj : m_objects)
        {
            if (obj.name == name)
                return obj;
        }
        return std::nullopt;
    }

    static inline Ref<Mesh> mesh;
    static inline Ref<Shader> shader;

private:
    String m_name;
    Vector<Object> m_objects;
    Vector<Animation> m_animation;
    Ref<Buffer> m_global_buffer;
};

class Animator
{
public:
    void set_model(Ref<Model> model);
    void play(String animation);
    void tick(float delta);

    struct TransformWithLength
    {
        Model::Transform transform;
        uint32_t frame_index = 0;
    };

private:
    Ref<Model> m_model;
    String m_animation_name;
    float m_time = 0.0;

    uint32_t m_frame;

    void update_model_animation_buffer();

    std::optional<Model::Keyframe> get_keyframe_for_frame(uint32_t frame) const;

    std::optional<TransformWithLength> get_current_transform(String object_name) const;
    std::optional<TransformWithLength> get_next_transform(String object_name) const;
};
