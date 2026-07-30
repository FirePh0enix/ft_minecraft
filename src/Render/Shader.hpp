#pragma once

#include "Core/Class.hpp"
#include "Core/Result.hpp"
#include "Render/Types.hpp"
#include "stdext.hpp"

#include <unordered_map>

class Shader : public Object
{
    CLASS(Shader, Object);

public:
    static Result<std::shared_ptr<Shader>> load(std::string_view source);
    static Result<std::shared_ptr<Shader>> load_from_path(std::string_view path);

    ~Shader();

    SamplerDescriptor get_sampler(std::string_view name) const
    {
        auto opt = m_samplers.find(name);
        if (opt != m_samplers.end())
            return opt->second;
        return SamplerDescriptor{};
    }

    std::optional<Binding> get_binding(std::string_view name) const
    {
        auto opt = m_bindings.find(name);
        if (opt != m_bindings.end())
            return opt->second;
        return std::nullopt;
    }

    void set_binding(std::string_view name, Binding binding)
    {
        m_bindings[std::string(name)] = binding;
    }

    bool has_binding(std::string_view name) const
    {
        return m_bindings.contains(name);
    }

    inline const stdext::string_map<Binding>& get_bindings() const
    {
        return m_bindings;
    }

    void set_sampler(std::string_view name, SamplerDescriptor sampler)
    {
        ERR_COND_V(!has_binding(name) || get_binding(name).value().kind != BindingKind::Texture, "binding `{}` is not a texture", name);
        m_samplers[std::string(name)] = sampler;
    }

    std::string_view get_source_string() const { return m_source_code; }
    std::string_view path() const { return m_path; }

    uint32_t hash() const { return m_hash; }
    WGPUBindGroupLayout get_bind_group_layout() const { return m_bind_group_layout; }
    WGPUPipelineLayout get_pipeline_layout() const { return m_pipeline_layout; }

    void create_bind_group_layout();

private:
    std::string m_path;
    std::string m_source_code;
    size_t m_size;

    uint32_t m_hash;

    stdext::string_map<Binding> m_bindings;
    stdext::string_map<SamplerDescriptor> m_samplers;

    WGPUBindGroupLayout m_bind_group_layout = nullptr;
    WGPUPipelineLayout m_pipeline_layout = nullptr;
};
