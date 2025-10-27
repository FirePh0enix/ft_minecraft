#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "Render/Types.hpp"

#include <filesystem>
#include <map>
#include <optional>

#include <slang-com-helper.h>
#include <slang-com-ptr.h>
#include <slang.h>

enum class ShaderFlagBits
{
    DepthPass = 1 << 0,
};
using ShaderFlags = Flags<ShaderFlagBits>;
DEFINE_FLAG_TRAITS(ShaderFlagBits);

struct PushConstantRange
{
    ShaderStageFlags stages;
    size_t size = 0;
};

class Shader : public Object
{
    CLASS(Shader, Object);

public:
    static Result<Ref<Shader>> load(const std::filesystem::path& path);

    ~Shader();

    Result<> dump_glsl();

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

    std::string get_entry_point(ShaderStageFlagBits stage) const
    {
        return m_entry_point_names.at(stage);
    }

    inline const std::vector<PushConstantRange>& get_push_constants() const
    {
        return m_push_constants;
    }

    ShaderStageFlags get_stage_mask() const
    {
        return m_stage_mask;
    }

    std::string path() const
    {
        return m_path;
    }

    inline View<char> get_code() const
    {
        return View((char *)m_code, m_size);
    }

    inline View<uint32_t> get_code_u32() const
    {
        return View((uint32_t *)m_code, m_size / sizeof(uint32_t));
    }

private:
    static inline Slang::ComPtr<slang::IGlobalSession> s_global_session;

    std::string m_path;
    std::string m_source_code;
    Slang::ComPtr<slang::ISession> m_session;
    Slang::ComPtr<slang::IModule> m_module;

    char *m_code = nullptr;
    size_t m_size;

    ShaderStageFlags m_stage_mask;

    std::map<std::string, Binding> m_bindings;
    std::map<std::string, SamplerDescriptor> m_samplers;
    std::vector<PushConstantRange> m_push_constants;

    std::map<ShaderStageFlagBits, std::string> m_entry_point_names;
};
