#pragma once

#include "Render/Types.hpp"

struct WGSLAttribs
{
    std::optional<uint32_t> group;
    std::optional<uint32_t> binding;
};

struct WGSLVar
{
    WGSLAttribs attribs;
};

struct WGSLModule
{
    static WGSLModule parse(const std::string& source);

    std::vector<WGSLVar> vars;
};
