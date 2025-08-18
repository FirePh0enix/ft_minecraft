#pragma once

#include "Core/Flags.hpp"

enum class VSync : uint8_t
{
    /**
     * @brief Disable vertical synchronization.
     */
    Off,

    /**
     * @brief Enable vertical synchronization.
     */
    On,
};

enum class BufferVisibility : uint8_t
{
    /**
     * @brief Indicate a buffer is only visible from GPU.
     */
    GPUOnly,

    /**
     * @brief Indicate a buffer visible from GPU and CPU.
     */
    GPUAndCPU,
};

enum class BufferUsageFlagBits : uint32_t
{
    /**
     * @brief The buffer can be used as source in copy operation.
     */
    CopySource = 1 << 0,

    /**
     * @brief The buffer can be used as destination in copy operation.
     */
    CopyDest = 1 << 1,

    /**
     * @brief The buffer can be used as a uniform in shader.
     */
    Uniform = 1 << 2,

    /**
     * @brief The buffer can be used as a index buffer in shader.
     */
    Index = 1 << 3,

    /**
     * @brief The buffer can be used as a vertex buffer in shader.
     */
    Vertex = 1 << 4,
};
using BufferUsageFlags = Flags<BufferUsageFlagBits>;
DEFINE_FLAG_TRAITS(BufferUsageFlagBits);

enum class TextureFormat : uint8_t
{
    R8Unorm,
    RGBA8Unorm,

    RGBA8Srgb,
    BGRA8Srgb,

    R32Sfloat,
    RG32Sfloat,
    RGB32Sfloat,
    RGBA32Sfloat,

    /**
     * @brief Depth 32-bits per pixel.
     */
    Depth32,
};

size_t size_of(const TextureFormat& format);

enum class TextureDimension : uint8_t
{
    D1D,
    D2D,
    D2DArray,
    D3D,
    Cube,
    CubeArray,
};

enum class TextureUsageFlagBits
{
    /**
     * @brief The texture is used as source in copy operation.
     */
    CopySource = 1 << 0,

    /**
     * @brief The texture is used as destination in copy operation.
     */
    CopyDest = 1 << 1,

    /**
     * @brief The texture can be sampled from shader.
     */
    Sampled = 1 << 2,

    /**
     * @brief The texture is use as color attachment in a framebuffer.
     */
    ColorAttachment = 1 << 3,

    /**
     * @brief The texture is use as depth attachment in a framebuffer.
     */
    DepthAttachment = 1 << 4,
};
using TextureUsageFlags = Flags<TextureUsageFlagBits>;
DEFINE_FLAG_TRAITS(TextureUsageFlagBits);

// TODO: This probably should not be exposed.
enum class TextureLayout : uint8_t
{
    Undefined,
    DepthStencilAttachment,
    CopyDst,
    ShaderReadOnly,
    DepthStencilReadOnly,
};

enum class IndexType : uint8_t
{
    Uint16,
    Uint32,
};

size_t size_of(const IndexType& format);

enum class PolygonMode : uint8_t
{
    Fill,
    Line,
    Point,
};

enum class CullMode : uint8_t
{
    None,
    Front,
    Back,
};

enum class ShaderType : uint8_t
{
    Float32,
    Float32x2,
    Float32x3,
    Float32x4,

    Uint32,
    Uint32x2,
    Uint32x3,
    Uint32x4,
};

enum class Filter : uint8_t
{
    Linear,
    Nearest,
};

enum class AddressMode : uint8_t
{
    Repeat,
    ClampToEdge,
};

struct SamplerDescriptor
{
    Filter min_filter = Filter::Linear;
    Filter mag_filter = Filter::Linear;

    struct
    {
        AddressMode u = AddressMode::Repeat;
        AddressMode v = AddressMode::Repeat;
        AddressMode w = AddressMode::Repeat;
    } address_mode = {};

    bool operator<(const SamplerDescriptor& o) const
    {
        return min_filter < o.min_filter || mag_filter < o.mag_filter || address_mode.u < o.address_mode.u || address_mode.v < o.address_mode.v || address_mode.w < o.address_mode.w;
    }
};

enum class ShaderStageFlagBits
{
    Vertex = 1 << 0,
    Fragment = 1 << 1,
    Compute = 1 << 2,
};
using ShaderStageFlags = Flags<ShaderStageFlagBits>;
DEFINE_FLAG_TRAITS(ShaderStageFlagBits);

enum class BindingKind : uint8_t
{
    Texture,
    UniformBuffer,
    StorageBuffer,
    PushConstants,
};

struct Binding
{
    BindingKind kind = BindingKind::Texture;
    ShaderStageFlags shader_stage = ShaderStageFlagBits::Vertex;
    uint32_t group = 0;
    uint32_t binding = 0;

    Binding()
    {
    }

    Binding(BindingKind kind, ShaderStageFlags shader_stage, uint32_t group, uint32_t binding)
        : kind(kind), shader_stage(shader_stage), group(group), binding(binding)
    {
    }

    Binding(BindingKind kind, ShaderStageFlags shader_stage, uint32_t group, uint32_t binding, TextureDimension dimension)
        : kind(kind), shader_stage(shader_stage), group(group), binding(binding), dimension(dimension)
    {
    }

    union
    {
        /**
         * Available only when binding is a `BindingKind::Texture`.
         */
        TextureDimension dimension = {};
    };
};
