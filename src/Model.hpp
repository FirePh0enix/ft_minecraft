#pragma once

#include "Core/Result.hpp"
#include "Transform3D.hpp"

#include <memory>
#include <span>

class Buffer;
class Texture;
class Material;
struct RenderPass;
class BindGroup;

class ModelLegacy
{
public:
    struct Info
    {
        glm::mat4 model_matrix;
    };

    struct Object
    {
        std::string name;

        glm::vec3 size;
        glm::vec3 position;
        glm::vec3 origin;

        std::shared_ptr<Buffer> model_buffer;
        std::shared_ptr<Buffer> uv_buffer;

        std::shared_ptr<BindGroup> bg;
    };

    struct Transform
    {
        std::string object_name;
        glm::vec3 position;
        glm::vec3 rotation;
    };

    struct Keyframe
    {
        uint32_t frame;
        std::vector<Transform> transforms;
    };

    struct Animation
    {
        std::string name;
        uint32_t fps;
        uint32_t frames;
        std::vector<Keyframe> keyframes;
    };

    static Result<std::shared_ptr<ModelLegacy>> load(std::string_view path);

    std::string_view name() const { return m_name; }
    std::span<const Object> objects() const;
    std::span<Object> objects();
    std::shared_ptr<Buffer> get_global_buffer() const;

    std::optional<Animation> get_animation(std::string_view name) const
    {
        for (const Animation& anim : m_animation)
        {
            if (anim.name == name)
                return anim;
        }
        return std::nullopt;
    }

    std::optional<Object> get_object(std::string_view name) const;

    void encode(const RenderPass& pass, const Transform3D& transform);

private:
    std::string m_name;
    std::vector<Object> m_objects;
    std::vector<Animation> m_animation;
    std::shared_ptr<Buffer> m_global_buffer;
    std::shared_ptr<Texture> m_texture;
};

class Animator
{
public:
    void set_model(std::shared_ptr<ModelLegacy> model);
    void play(const std::string& animation);
    void tick(float delta);

    struct TransformWithLength
    {
        ModelLegacy::Transform transform;
        uint32_t frame_index = 0;
    };

private:
    std::shared_ptr<ModelLegacy> m_model;
    std::string m_animation_name;
    float m_time = 0.0;

    uint32_t m_frame;

    void update_model_animation_buffer();

    std::optional<ModelLegacy::Keyframe> get_keyframe_for_frame(uint32_t frame) const;

    std::optional<TransformWithLength> get_current_transform(std::string_view object_name) const;
    std::optional<TransformWithLength> get_next_transform(std::string_view object_name) const;
};
