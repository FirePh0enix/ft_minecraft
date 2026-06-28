#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Render/Types.hpp"

class Shader : public Object
{
    CLASS(Shader, Object);

public:
    static Result<Ref<Shader>> load(const StringView& source);
    static Result<Ref<Shader>> load_from_path(const StringView& path);

    ~Shader();

    SamplerDescriptor get_sampler(const String& name) const
    {
        auto opt = m_samplers.get(name);
        if (opt.has_value())
            return opt.value();
        return SamplerDescriptor{};
    }

    Option<Binding> get_binding(const String& name) const
    {
        return m_bindings.get(name);
    }

    void set_binding(const String& name, Binding binding)
    {
        m_bindings.put(name, binding);
    }

    bool has_binding(const String& name) const
    {
        return m_bindings.contains(name);
    }

    inline const HashMap<String, Binding>& get_bindings() const
    {
        return m_bindings;
    }

    void set_sampler(const String& name, SamplerDescriptor sampler)
    {
        ERR_COND_V(!has_binding(name) || get_binding(name).value().kind != BindingKind::Texture, "binding `{}` is not a texture", name);
        m_samplers.put(name, sampler);
    }

    StringView get_source_string() const { return m_source_code; }

    String path() const
    {
        return m_path;
    }

    uint32_t hash() const { return m_hash; }
    WGPUBindGroupLayout get_bind_group_layout() const { return m_bind_group_layout; }
    WGPUPipelineLayout get_pipeline_layout() const { return m_pipeline_layout; }

    void create_bind_group_layout();

private:
    String m_path;
    String m_source_code;
    size_t m_size;

    uint32_t m_hash;

    HashMap<String, Binding> m_bindings;
    HashMap<String, SamplerDescriptor> m_samplers;

    WGPUBindGroupLayout m_bind_group_layout = nullptr;
    WGPUPipelineLayout m_pipeline_layout = nullptr;
};
