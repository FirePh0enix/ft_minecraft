#pragma once

#include "Core/Class.hpp"
#include "Scene/Components/Component.hpp"
#include "Scene/Entity.hpp"

class Transform3D
{
public:
    Transform3D()
        : m_position(0.0), m_rotation(glm::identity<glm::quat>()), m_scale(1.0)
    {
    }

    Transform3D(glm::vec3 position, glm::quat rotation = glm::identity<glm::quat>(), glm::vec3 scale = glm::vec3(1.0))
        : m_position(position), m_rotation(rotation), m_scale(scale)
    {
    }

    Transform3D with_parent(const Transform3D& parent_transform) const
    {
        Transform3D transform = Transform3D::from_matrix(parent_transform.to_matrix() * to_matrix());

        transform.m_rotation = m_rotation * parent_transform.m_rotation;

        return transform;
    }

    inline const glm::vec3& position() const
    {
        return m_position;
    }

    inline glm::vec3& position()
    {
        return m_position;
    }

    inline const glm::quat& rotation() const
    {
        return m_rotation;
    }

    inline glm::quat& rotation()
    {
        return m_rotation;
    }

    inline const glm::vec3& scale() const
    {
        return m_scale;
    }

    inline glm::vec3& scale()
    {
        return m_scale;
    }

    /**
     * Rotate a vector using this transform's rotation.
     */
    inline glm::vec3 rotate(const glm::vec3& v) const
    {
        return glm::conjugate(m_rotation) * v;
    }

    /**
     * Returns the forward (negative Z) vector local to this transform.
     */
    inline glm::vec3 forward() const
    {
        return rotate(glm::vec3(0.0, 0.0, -1.0));
    }

    /**
     * Returns the right (positive X) vector local to this transform.
     */
    inline glm::vec3 right() const
    {
        return rotate(glm::vec3(1.0, 0.0, 0.0));
    }

    /**
     * Returns this transform's rotation as euler angles.
     */
    inline glm::vec3 euler_angles() const
    {
        return glm::eulerAngles(m_rotation);
    }

    inline void set_euler_angles(const glm::vec3& euler_angles)
    {
        m_rotation = glm::quat(euler_angles);
    }

    inline void set_position(glm::vec3 pos)
    {
        m_position = pos;
    }

private:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    glm::mat4 to_matrix() const
    {
        return glm::translate(glm::mat4(1.0), m_position) * glm::mat4_cast(m_rotation) * glm::scale(glm::mat4(1.0), m_scale);
    }

    static Transform3D from_matrix(glm::mat4 matrix)
    {
        glm::vec3 position(matrix[3][0], matrix[3][1], matrix[3][2]);
        glm::quat rotation = glm::quat_cast(matrix);
        glm::vec3 scale = glm::vec3(1.0);

        return Transform3D(position, rotation, scale);
    }
};

class TransformComponent3D : public Component
{
    CLASS(Transform3D, Component);

public:
    TransformComponent3D()
    {
    }

    TransformComponent3D(const Transform3D& transform)
        : m_transform(transform)
    {
    }

    virtual void start() override
    {
        if (m_entity->has_parent())
            m_parent_transform = m_entity->get_parent()->get_component<TransformComponent3D>();
    }

    inline Transform3D get_transform() const
    {
        return m_transform;
    }

    void set_transform(const Transform3D& transform)
    {
        m_transform = transform;
    }

    Transform3D get_global_transform() const
    {
        if (m_parent_transform.is_null())
        {
            return get_transform();
        }

        return get_transform().with_parent(m_entity->get_parent()->get_component<TransformComponent3D>()->get_global_transform());
    }

private:
    Transform3D m_transform;
    Ref<TransformComponent3D> m_parent_transform;
};
