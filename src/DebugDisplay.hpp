#pragma once

#include "Color.hpp"
#include "Core/Math.hpp"

#include <map>
#include <memory>

class Buffer;
class BindGroup;
struct RenderPass;

struct DebugShape
{
    virtual ~DebugShape() {}

    float creation_time;
    float duration;

    std::shared_ptr<Buffer> buffer;
    std::shared_ptr<BindGroup> bg;

    virtual void draw(const RenderPass& pass) const = 0;

    glm::dvec3 position;
    glm::vec3 scale;
};

struct DebugCube : public DebugShape
{
    DebugCube(glm::dvec3 position, glm::vec3 scale, Color color, float duration, float creation_time);

    virtual void draw(const RenderPass& pass) const override;

    Color color;
};

class DebugDisplay
{
public:
    DebugDisplay();

    void update(float delta);
    void draw(const RenderPass& pass);

    void draw_cube(glm::dvec3 position, glm::vec3 size, Color color = Colors::yellow, float duration = 10.0f);

private:
    float m_timer = 0.0f;
    std::map<uint64_t, std::unique_ptr<DebugShape>> m_shapes;
    uint64_t m_id = 0;
};
