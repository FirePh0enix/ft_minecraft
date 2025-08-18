#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "Render/Types.hpp"

#include <map>

enum class ShaderFlagBits
{
    DepthPass = 1 << 0,
};
using ShaderFlags = Flags<ShaderFlagBits>;
DEFINE_FLAG_TRAITS(ShaderFlagBits);

// TODO: Replace `ShaderFlags`
enum class ShaderVariant
{
    DepthPass,
};

class Shader : public Object
{
    CLASS(Shader, Object);

public:
    enum class ErrorKind : uint8_t
    {
        FileNotFound,
        MissingDirective,
        RecursiveDirective,
        Compilation,
    };

    template <typename T = char>
    using Result = Result<T, ErrorKind>;

    static Result<Ref<Shader>> compile(const std::string& filename, ShaderFlags flags = ShaderFlags());

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

    SamplerDescriptor get_sampler(const std::string& name) const
    {
        auto iter = m_samplers.find(name);
        if (iter != m_samplers.end())
        {
            return iter->second;
        }
        return SamplerDescriptor{};
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

#ifdef __platform_web
    inline std::string get_code(ShaderStageFlagBits stage)
    {
        return m_stages.at(stage);
    }
#else
    inline std::vector<uint32_t> get_code(ShaderStageFlagBits stage) const
    {
        return m_stages.at(stage);
    }
#endif

private:
    std::map<std::string, Binding> m_bindings;
    std::map<std::string, SamplerDescriptor> m_samplers;
    ShaderStageFlags m_stage_mask;

    Result<> compile_internal(const std::string& filename, ShaderFlags flags);

#ifdef __platform_web
    std::map<ShaderStageFlagBits, std::string> m_stages;
#else
    std::map<ShaderStageFlagBits, std::vector<uint32_t>> m_stages;
#endif
};
