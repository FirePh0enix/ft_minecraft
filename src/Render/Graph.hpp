#pragma once

#include "Core/Span.hpp"

class Buffer;
class Mesh;
class Material;

enum class InstructionKind : uint8_t
{
    BeginRenderPass,
    EndRenderPass,
    Draw,
    Copy,
};

union Instruction
{
    InstructionKind kind;
    struct
    {
        InstructionKind kind;
    } renderpass;
    struct
    {
        InstructionKind kind;
        Mesh *mesh;
        Material *material;
        size_t instance_count;
        std::optional<Buffer *> instance_buffer;
        glm::mat4 view_matrix;
    } draw;
    struct
    {
        InstructionKind kind;
        Buffer *src;
        Buffer *dst;
        size_t src_offset;
        size_t dst_offset;
        size_t size;
    } copy;
};

struct PushConstants
{
    glm::mat4 view_matrix;
};

class RenderGraph
{
public:
    RenderGraph();

    void reset();
    Span<Instruction> get_instructions() const;

    void begin_render_pass();
    void end_render_pass();

    void add_draw(Mesh *mesh, Material *material, glm::mat4 view_matrix = {}, uint32_t instance_count = 1, std::optional<Buffer *> instance_buffer = {});
    void add_copy(Buffer *src, Buffer *dst, size_t size, size_t src_offset = 0, size_t dst_offset = 0);

private:
    std::vector<Instruction> m_instructions;
    bool m_renderpass;
};
