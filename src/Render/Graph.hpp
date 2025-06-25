#pragma once

#include "Core/Span.hpp"

class Buffer;
class Mesh;
class Material;

enum class InstructionKind : uint8_t
{
    BeginRenderPass,
    EndRenderPass,
    BeginDepthPass,
    EndDepthPass,
    Draw,
    Copy,
};

union Instruction
{
    InstructionKind kind = {};
    struct
    {
        InstructionKind kind = InstructionKind::BeginRenderPass;
        bool use_previous_depth_pass = false;
    } renderpass;
    struct
    {
        InstructionKind kind = InstructionKind::BeginDepthPass;
    } depthpass;
    struct
    {
        InstructionKind kind = InstructionKind::Draw;
        Mesh *mesh = nullptr;
        Material *material = nullptr;
        size_t instance_count = 0;
        std::optional<Buffer *> instance_buffer = std::nullopt;
        glm::mat4 view_matrix = glm::mat4(1.0);
        bool ignore_depth_prepass = false;
    } draw;
    struct
    {
        InstructionKind kind = InstructionKind::Copy;
        Buffer *src = nullptr;
        Buffer *dst = nullptr;
        size_t src_offset = 0;
        size_t dst_offset = 0;
        size_t size = 0;
    } copy;
};

struct PushConstants
{
    glm::mat4 view_matrix;
    // NaN does not exists with WGSL.
    glm::float32 nan;
};

class RenderGraph
{
public:
    RenderGraph();

    void reset();
    Span<Instruction> get_instructions() const;

    /**
     * @brief Start a depth-only render pass.
     */
    void begin_depth_pass();
    void end_depth_pass();

    /**
     * @brief Start a standard render pass.
     */
    void begin_render_pass(bool use_previous_depth_pass = false);
    void end_render_pass();

    void add_draw(Mesh *mesh, Material *material, glm::mat4 view_matrix = {}, uint32_t instance_count = 1, std::optional<Buffer *> instance_buffer = {}, bool ignore_depth_prepass = false);
    void add_copy(Buffer *src, Buffer *dst, size_t size, size_t src_offset = 0, size_t dst_offset = 0);

private:
    std::vector<Instruction> m_instructions;
    bool m_renderpass = false;
};
