#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "Render/Types.hpp"

#include <filesystem>
#include <map>

#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>

enum class ShaderFlagBits
{
    DepthPass = 1 << 0,
};
using ShaderFlags = Flags<ShaderFlagBits>;
DEFINE_FLAG_TRAITS(ShaderFlagBits);

class Shader : public Object
{
    CLASS(Shader, Object);

public:
    static Result<Ref<Shader>> load(const std::filesystem::path& path);

    SamplerDescriptor get_sampler(const std::string& name) const
    {
        auto iter = m_samplers.find(name);
        if (iter != m_samplers.end())
        {
            return iter->second;
        }
        return SamplerDescriptor{};
    }

    std::optional<Binding> get_binding(const std::string& name) const
    {
        const auto& iter = m_bindings.find(name);
        if (iter == m_bindings.end())
            return std::nullopt;
        return iter->second;
    }

    void set_binding(const std::string& name, Binding binding)
    {
        m_bindings[name] = binding;
    }

    bool has_binding(const std::string& name) const
    {
        return m_bindings.find(name) != m_bindings.end();
    }

    inline const std::map<std::string, Binding>& get_bindings() const
    {
        return m_bindings;
    }

    void set_sampler(const std::string& name, SamplerDescriptor sampler)
    {
        ERR_COND_V(!has_binding(name) || get_binding(name)->kind != BindingKind::Texture, "binding `{}` is not a texture", name);
        m_samplers[name] = sampler;
    }

    ShaderStageFlags get_stage_mask() const
    {
        return m_stage_mask;
    }

#ifndef __platform_web
    inline const std::vector<uint32_t> get_code() const
    {
        return m_binary_code;
    }
#else
    inline const std::string get_code() const
    {
        return m_binary_code;
    }
#endif

private:
    static inline Slang::ComPtr<slang::IGlobalSession> s_global_session;

    std::string m_source_code;
    Slang::ComPtr<slang::ISession> m_session;
    Slang::ComPtr<slang::IModule> m_module;

#ifndef __platform_web
    std::vector<uint32_t> m_binary_code;
#else
    std::string m_shader_code;
#endif

    ShaderStageFlags m_stage_mask;

    std::map<std::string, Binding> m_bindings;
    std::map<std::string, SamplerDescriptor> m_samplers;

    static void dump_glsl(const std::filesystem::path& path);
};
