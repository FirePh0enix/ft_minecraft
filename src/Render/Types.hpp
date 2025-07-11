#pragma once

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

struct BufferUsage
{
    /**
     * @brief Used as source in copy operation.
     */
    bool copy_src : 1 = false;

    /**
     * @brief Used as destination in copy operation.
     */
    bool copy_dst : 1 = false;

    /**
     * @brief Used as an uniform buffer.
     */
    bool uniform : 1 = false;

    /**
     * @brief Used as an index buffer.
     */
    bool index : 1 = false;

    /**
     * @brief Used as an vertex or instance buffer.
     */
    bool vertex : 1 = false;
};

inline bool operator<(const BufferUsage& a, const BufferUsage& b)
{
    return std::tie(a.copy_src, a.copy_dst, a.uniform, a.index, a.vertex) < std::tie(b.copy_src, b.copy_dst, b.uniform, b.index, b.vertex);
}

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

struct TextureUsage
{
    /**
     * @brief Used as source in copy operation.
     */
    bool copy_src : 1 = false;

    /**
     * @brief Used as destination in copy operation.
     */
    bool copy_dst : 1 = false;

    bool sampled : 1 = false;

    bool color_attachment : 1 = false;
    bool depth_attachment : 1 = false;
};

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

enum class ShaderStageKind : uint8_t
{
    Vertex,
    Fragment,
    Compute,
};

enum class BindingKind : uint8_t
{
    Texture,
    UniformBuffer,
};

struct Binding
{
    BindingKind kind;
    ShaderStageKind shader_stage = ShaderStageKind::Vertex;
    uint32_t group;
    uint32_t binding;

    union
    {
        /**
         * Available only when binding is a `BindingKind::Texture`.
         */
        TextureDimension dimension = {};
    };
};
