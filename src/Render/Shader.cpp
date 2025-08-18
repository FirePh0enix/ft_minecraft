#include "Render/Shader.hpp"
#include "Core/Filesystem.hpp"
#include "Render/ShaderMap.hpp"

// TODO: add shader variants back.

Shader::Result<Ref<Shader>> Shader::compile(const std::string& filename, ShaderFlags flags)
{
    Ref<Shader> shader = make_ref<Shader>();
    auto result = shader->compile_internal(filename, flags);
    YEET(result);

    return shader;
}

Shader::Result<> Shader::compile_internal(const std::string& filename, ShaderFlags flags)
{
    std::vector<std::string> definitions;

    if (flags.has_any(ShaderFlagBits::DepthPass))
    {
        definitions.push_back("DEPTH_PASS");
    }

    std::string map_source = Filesystem::read_file_to_string("build/" + filename + ".map.json"); // TODO: `Should find a way to remove the `build/` at the start.
    nlohmann::json j = nlohmann::json::parse(map_source, nullptr, false, false);
    ShaderMap map = j;

    for (const ShaderMap::Stage& stage : map.stages)
    {
        std::vector<char> buffer = Filesystem::read_file_to_buffer("build/" + stage.file);
        assert(buffer.size() % 4 == 0 && "SPIR-V file size must be a multiple of 4");

        std::vector<uint32_t> buffer_u32(buffer.size() / 4);
        std::memcpy(buffer_u32.data(), buffer.data(), buffer.size());

        m_stages[stage.stage] = buffer_u32;
        m_stage_mask |= stage.stage;
    }

    for (const ShaderMap::Uniform& uniform : map.uniforms)
    {
        set_binding(uniform.name, Binding(uniform.type, uniform.stage, uniform.set, uniform.binding));
    }

    return 0;
}
