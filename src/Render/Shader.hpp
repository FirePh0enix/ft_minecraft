#pragma once

#include "Core/Class.hpp"
#include "Core/Containers/View.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "Render/Types.hpp"

#include <filesystem>
#include <map>
#include <optional>

enum class ShaderFlagBits
{
    DepthPass = 1 << 0,
};
using ShaderFlags = Flags<ShaderFlagBits>;
DEFINE_FLAG_TRAITS(ShaderFlagBits);

enum class ShaderKind
{
    SPIRV,
    WGSL,
    GLSL,
};

class Shader : public Object
{
    CLASS(Shader, Object);

public:
    static Result<Ref<Shader>> load(const std::filesystem::path& path);

    ~Shader();

    SamplerDescriptor get_sampler(const String& name) const
    {
        auto iter = m_samplers.find(name);
        if (iter != m_samplers.end())
        {
            return iter->second;
        }
        return SamplerDescriptor{};
    }

    std::optional<Binding> get_binding(const String& name) const
    {
        const auto& iter = m_bindings.find(name);
        if (iter == m_bindings.end())
            return std::nullopt;
        return iter->second;
    }

    void set_binding(const String& name, Binding binding)
    {
        m_bindings[name] = binding;
    }

    bool has_binding(const String& name) const
    {
        return m_bindings.find(name) != m_bindings.end();
    }

    inline const std::map<String, Binding>& get_bindings() const
    {
        return m_bindings;
    }

    void set_sampler(const String& name, SamplerDescriptor sampler)
    {
        ERR_COND_V(!has_binding(name) || get_binding(name)->kind != BindingKind::Texture, "binding `{}` is not a texture", name);
        m_samplers[name] = sampler;
    }

    StringView get_source_string() const { return m_source_code; }
    ShaderKind get_shader_kind() const { return m_kind; }

    String get_entry_point(ShaderStageFlagBits stage) const
    {
        return m_entry_point_names.at(stage);
    }

    ShaderStageFlags get_stage_mask() const
    {
        return m_stage_mask;
    }

    String path() const
    {
        return m_path;
    }

    uint32_t hash() const { return m_hash; }

private:
    ShaderKind m_kind;
    String m_path;
    String m_source_code;
    size_t m_size;

    uint32_t m_hash;

    ShaderStageFlags m_stage_mask;

    std::map<String, Binding> m_bindings;
    std::map<String, SamplerDescriptor> m_samplers;

    std::map<ShaderStageFlagBits, String> m_entry_point_names;

    static Result<Ref<Shader>> load_glsl_shader(const std::filesystem::path& path);
    static Result<Ref<Shader>> load_slang_shader(const std::filesystem::path& path, bool compute_shader);
};
