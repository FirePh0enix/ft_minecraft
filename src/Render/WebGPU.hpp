#pragma once

#include <webgpu/webgpu.h>

namespace wgpu
{

enum class TextureViewDimension : int
{
    Undefined = WGPUTextureViewDimension_Undefined,
    D1D = WGPUTextureViewDimension_1D,
    D2D = WGPUTextureViewDimension_2D,
    D2DArray = WGPUTextureViewDimension_2DArray,
    Cube = WGPUTextureViewDimension_Cube,
    CubeArray = WGPUTextureViewDimension_CubeArray,
    D3D = WGPUTextureViewDimension_3D,
};

struct TextureViewDescriptor : WGPUTextureViewDescriptor
{
    TextureViewDescriptor()
        : WGPUTextureViewDescriptor{}
    {
        baseMipLevel = 0;
        mipLevelCount = 1;
        baseArrayLayer = 0;
        arrayLayerCount = 1;
    }

    TextureViewDescriptor& next(WGPUChainedStruct& next_in_chain)
    {
        this->nextInChain = &next_in_chain;
        return *this;
    }

    TextureViewDescriptor& with_format(WGPUTextureFormat format)
    {
        this->format = format;
        return *this;
    }

    TextureViewDescriptor& with_dimension(TextureViewDimension dimension)
    {
        this->dimension = WGPUTextureViewDimension(dimension);
        return *this;
    }

    TextureViewDescriptor& with_mip_level(uint32_t base, uint32_t count)
    {
        this->baseMipLevel = base;
        this->mipLevelCount = count;
        return *this;
    }

    TextureViewDescriptor& with_array_layers(uint32_t base, uint32_t count)
    {
        this->baseArrayLayer = base;
        this->arrayLayerCount = count;
        return *this;
    }

    TextureViewDescriptor& with_aspect(WGPUTextureAspect aspect)
    {
        this->aspect = aspect;
        return *this;
    }

    TextureViewDescriptor& with_usage(WGPUTextureUsage usage)
    {
        this->usage = usage;
        return *this;
    }
};

struct BindGroupEntry : WGPUBindGroupEntry
{
    BindGroupEntry()
        : WGPUBindGroupEntry{}
    {
    }

    BindGroupEntry& next(WGPUChainedStruct& next_in_chain)
    {
        this->nextInChain = &next_in_chain;
        return *this;
    }

    BindGroupEntry& with_binding(uint32_t binding)
    {
        this->binding = binding;
        return *this;
    }

    BindGroupEntry& with_buffer(WGPUBuffer buffer, uint64_t offset, uint64_t size)
    {
        this->buffer = buffer;
        this->offset = offset;
        this->size = size;
        return *this;
    }

    BindGroupEntry& with_texture_view(WGPUTextureView view)
    {
        this->textureView = view;
        return *this;
    }

    BindGroupEntry& with_sampler(WGPUSampler sampler)
    {
        this->sampler = sampler;
        return *this;
    }
};

} // namespace wgpu
