#include "Render/Shader.hpp"
#include "Core/Assert.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Hash.hpp"
#include "Core/Print.hpp"

Shader::~Shader()
{
}

Result<Ref<Shader>> Shader::load(const std::filesystem::path& path)
{
    // if (path.extension() == ".glsl" || path.extension() == ".comp" || path.extension() == ".vert" || path.extension() == ".frag")
    //     return load_glsl_shader(path);
    // return load_slang_shader(path, compute_shader);

    String source_code = Filesystem::open_file(path).value().read_to_string();

    Ref<Shader> shader = newobj(Shader);
    shader->m_source_code = source_code;
    shader->m_hash = hash_fnv32(source_code);
    shader->m_kind = ShaderKind::WGSL;
    shader->m_entry_point_names[ShaderStageFlagBits::Vertex] = "vertex_main";
    shader->m_entry_point_names[ShaderStageFlagBits::Fragment] = "fragment_main";

    return shader;
}
