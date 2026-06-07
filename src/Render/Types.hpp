#pragma once

#include <webgpu/webgpu.h>

#include <cstdint>
#include <tuple>

enum class TextureDimension : uint8_t
{
    D1D,
    D2D,
    D2DArray,
    D3D,
    Cube,
    CubeArray,
};

enum class UVType : uint8_t
{
    /**
     *  Standard texture coordinates.
     */
    UV,

    /**
     *  Texture coordinates also store an index into a texture array as `Z`.
     */
    UVT,
};

struct SamplerDescriptor
{
    WGPUFilterMode min_filter = WGPUFilterMode_Linear;
    WGPUFilterMode mag_filter = WGPUFilterMode_Linear;

    struct
    {
        WGPUAddressMode u = WGPUAddressMode_Repeat;
        WGPUAddressMode v = WGPUAddressMode_Repeat;
        WGPUAddressMode w = WGPUAddressMode_Repeat;
    } address_mode = {};

    bool operator<(const SamplerDescriptor& o) const
    {
        return std::tie(min_filter, mag_filter, address_mode.u, address_mode.v, address_mode.w) < std::tie(o.min_filter, o.mag_filter, o.address_mode.u, o.address_mode.v, o.address_mode.w);
    }

    bool operator>(const SamplerDescriptor& o) const
    {
        return std::tie(min_filter, mag_filter, address_mode.u, address_mode.v, address_mode.w) > std::tie(o.min_filter, o.mag_filter, o.address_mode.u, o.address_mode.v, o.address_mode.w);
    }

    bool operator==(const SamplerDescriptor& o) const
    {
        return std::tie(min_filter, mag_filter, address_mode.u, address_mode.v, address_mode.w) == std::tie(o.min_filter, o.mag_filter, o.address_mode.u, o.address_mode.v, o.address_mode.w);
    }
};

enum class BindingKind : uint8_t
{
    Texture,
    StorageTexture,
    UniformBuffer,
    StorageBuffer,
};

enum class BindingAccess
{
    Read,
    ReadWrite,
};

struct Binding
{
    BindingKind kind = BindingKind::Texture;
    WGPUShaderStage shader_stage = WGPUShaderStage_None;
    uint32_t group = 0;
    uint32_t binding = 0;
    BindingAccess access = BindingAccess::Read;

    Binding()
    {
    }

    Binding(BindingKind kind, WGPUShaderStage shader_stage, uint32_t group, uint32_t binding, BindingAccess access, TextureDimension dimension)
        : kind(kind), shader_stage(shader_stage), group(group), binding(binding), access(access), dimension(dimension)
    {
    }

    Binding(BindingKind kind, WGPUShaderStage shader_stage, uint32_t group, uint32_t binding, BindingAccess access)
        : kind(kind), shader_stage(shader_stage), group(group), binding(binding), access(access)
    {
    }

    union
    {
        /**
         * Available only when binding is a `Texture`.
         */
        TextureDimension dimension = {};
    };
};
