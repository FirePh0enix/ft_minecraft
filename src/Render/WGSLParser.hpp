#pragma once

#include "Core/String.hpp"

#include <cstdint>
#include <vector>

enum class WGSLTypeKind
{
    Primitive,
    Array,
    Texture,
    Sampler,
    Vec2
};

enum class WGSLDimension
{
    e1D,
    e2D,
    e3D,
    e4D,
};

struct WGSLType
{
    WGSLTypeKind kind;
    union
    {
        struct
        {
            WGSLType *child = nullptr;
            WGSLDimension dimension = WGSLDimension::e1D;
        } texture;
        WGSLType *child = nullptr;
        uint32_t stride;
    };

    ~WGSLType()
    {
        switch (kind)
        {
        case WGSLTypeKind::Array:
            destroy(child);
            break;
        case WGSLTypeKind::Texture:
            destroy(texture.child);
            break;
        default:
            break;
        }
    }
};

enum class WGSLAddressSpace
{
    Uniform,
    Storage,
    PushConstants,
};

struct WGSLVar
{
    WGSLAddressSpace address_space;
    uint32_t group;
    uint32_t binding;
    String name;
    WGSLType *type;

    ~WGSLVar()
    {
        if (type)
            destroy(type);
    }
};

class WGSLParser
{
public:
    WGSLParser(const StringView& source);

private:
    std::vector<WGSLVar> m_vars;
};
