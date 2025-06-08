#pragma once

struct WGSLAttribs
{
    std::optional<uint32_t> group;
    std::optional<uint32_t> binding;
    std::optional<uint32_t> location;
    std::optional<std::string> builtin;
};

struct WGSLType
{
    std::string name;
    std::vector<std::string> args;
};

struct WGSLVar
{
    WGSLAttribs attribs;
    std::string name;
    std::optional<std::string> address_space;
};

struct WGSLModule
{
    static WGSLModule parse(const std::string& source);

    std::vector<WGSLVar> vars;
};
