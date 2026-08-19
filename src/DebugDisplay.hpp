#pragma once

#include "Color.hpp"
#include "Core/Math.hpp"

#include <memory>
#include <vector>

class Buffer;
class BindGroup;
struct RenderPass;

struct DebugShape
{
    float creation_time;
    float duration;

    std::shared_ptr<Buffer> buffer;
    std::shared_ptr<BindGroup> bg;

    virtual void draw(const RenderPass& pass) const = 0;
};

struct DebugCube : public DebugShape
{
    DebugCube(glm::mat4 model, Color color, float duration, float creation_time);

    virtual void draw(const RenderPass& pass) const override;
};

class DebugDisplay
{
public:
    DebugDisplay();

    void update(float delta);
    void draw(const RenderPass& pass);

    void draw_cube(glm::vec3 position, glm::vec3 size, Color color = Colors::yellow, float duration = 10.0f);

private:
    float m_timer = 0.0f;
    std::vector<std::unique_ptr<DebugShape>> m_shapes;
};
