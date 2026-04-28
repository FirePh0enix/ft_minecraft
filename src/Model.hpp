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

        // Instance of `Info`.
        Ref<Buffer> model_buffer;
        Ref<Buffer> animation_buffer;
        Ref<Material> material;
    };

    struct Transform
    {
        String object_name;
        glm::vec3 position;
        glm::vec3 rotation;
    };

    struct Action
    {
        float time;
        Vector<Transform> transforms;
    };

    struct Animation
    {
        String name;
        Vector<Action> actions;
    };

    static Result<Ref<Model>> load(const StringView& path);

    String name() const { return m_name; }
    View<Object> objects() const { return m_objects; }
    Ref<Buffer> get_global_buffer() const { return m_global_buffer; }

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

    // This is a simple 1x1x1 cube scaled when rendering the model.
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

private:
    Ref<Model> m_model;
    String m_animation_name;
    float m_time = 0.0;
    uint32_t m_animation_index = 0;

    std::map<std::string, Model::Transform> m_previous_transforms;
};
