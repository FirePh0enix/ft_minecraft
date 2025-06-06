#pragma once

#include "Core/Class.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"
#include "Render/Types.hpp"

#include <map>

#ifndef __platform_web
#include <tint/tint.h>
#endif

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

    template <typename T>
    using Expected = std::expected<T, ErrorKind>;

    static Expected<Ref<Shader>> compile(const std::string& filename, const std::vector<std::string>& definitions);

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

    Sampler get_sampler(const std::string& name) const
    {
        auto iter = m_samplers.find(name);
        if (iter != m_samplers.end())
            return iter->second;
        return Sampler{};
    }

    void set_sampler(const std::string& name, Sampler sampler)
    {
        ERR_COND_V(!has_binding(name) || get_binding(name)->kind != BindingKind::Texture, "binding `{}` is not a texture", name);
        m_samplers[name] = sampler;
    }

#ifdef __has_shader_hot_reload
    void reload_if_needed();

    inline bool was_reloaded() const
    {
        return m_was_reloaded;
    }

    inline void set_was_reloaded(bool v)
    {
        m_was_reloaded = v;
    }

    static std::vector<Ref<Shader>> shaders;
#endif

#ifdef __platform_web
    inline std::string get_code()
    {
        return m_code;
    }
#else
    inline std::vector<uint32_t> get_code()
    {
        return m_code;
    }
#endif

private:
    std::map<std::string, Binding> m_bindings;
    std::map<std::string, Sampler> m_samplers;

    Shader::Expected<void> compile_internal(const std::string& filename, const std::vector<std::string>& definitions);

#ifdef __platform_web
    std::string m_code;
#else
    std::vector<uint32_t> m_code;

    void fill_info(const tint::core::ir::Module& ir);
    std::optional<ShaderStageKind> stage_of(const tint::core::ir::Module& ir, const tint::core::ir::Instruction *inst);
#endif

#ifdef __has_shader_hot_reload
    std::string m_filename;
    std::chrono::time_point<std::chrono::file_clock> m_file_last_write;
    std::vector<std::string> m_definitions;
    bool m_was_reloaded = false;
#endif
};
