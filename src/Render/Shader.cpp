#include "Render/Shader.hpp"

#include "Core/Assert.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Hash.hpp"
#include "Core/Print.hpp"
#include "Render/Renderer.hpp"
#include "Render/Types.hpp"

Shader::~Shader()
{
    if (m_bind_group_layout)
        wgpuBindGroupLayoutRelease(m_bind_group_layout);
    if (m_pipeline_layout)
        wgpuPipelineLayoutRelease(m_pipeline_layout);
}

static Result<String> preprocess(const String& source, StringView basepath)
{
    std::string source2(source.data(), source.size());
    std::stringstream ss(source2);
    std::string s;
    std::string line;

    while (std::getline(ss, line))
    {
	if (line.starts_with("#include "))
	{
	    std::string path = line.substr(10, line.length() - 11);

	    std::string fpath = basepath.data();
	    fpath += path;

	    String include_source = TRY(TRY(Filesystem::open_file(StringView(fpath.data(), fpath.size()))).reader().read_to_string());
	    
	    s.append(include_source.data(), include_source.size());
	    s.append("\n");
	}
	else
	{
	    s.append(line.data(), line.size());
	    s.append("\n");
	}
    }

    return String(s.data());
}

Result<Ref<Shader>> Shader::load(const StringView& source)
{
    Ref<Shader> shader = newref<Shader>();
    shader->m_source_code = source;
    shader->m_hash = hash_fnv32(source);
    return shader;
}

Result<Ref<Shader>> Shader::load_from_path(const StringView& path)
{
    File file = TRY(Filesystem::open_file(path));
    String s = TRY(file.reader().read_to_string());
    s = TRY(preprocess(s, "assets/shaders/"));
    return load(s);
}

static Vector<WGPUBindGroupLayoutEntry> convert_bindings(Shader *shader)
{
    Vector<WGPUBindGroupLayoutEntry> entries;
    entries.reserve(shader->get_bindings().size());

    for (const auto& [_, key, binding] : shader->get_bindings())
    {
        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = binding.shader_stage;
            entry.texture.sampleType = binding.sampler_binding == WGPUSamplerBindingType_NonFiltering ? WGPUTextureSampleType_UnfilterableFloat : WGPUTextureSampleType_Float;
            entry.texture.sampleType = binding.texture_sample;
            entry.texture.multisampled = false;
            entry.texture.viewDimension = binding.dimension;

            entries.append(entry);

            WGPUBindGroupLayoutEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.visibility = binding.shader_stage;
            sampler_entry.sampler.nextInChain = nullptr;
            sampler_entry.sampler.type = binding.sampler_binding;

            entries.append(sampler_entry);
        }
        break;
        case BindingKind::UniformBuffer:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = binding.shader_stage;
            entry.buffer.type = WGPUBufferBindingType_Uniform;

            entries.append(entry);
        }
        break;
        default:
            ASSERT(false, "NOT IMPLEMENTED");
            break;
        }
    }

    return entries;
}

void Shader::create_bind_group_layout()
{
    Vector<WGPUBindGroupLayoutEntry> entries = convert_bindings(this);

    WGPUBindGroupLayoutDescriptor bind_group_desc = WGPU_BIND_GROUP_LAYOUT_DESCRIPTOR_INIT;
    bind_group_desc.entryCount = entries.size();
    bind_group_desc.entries = entries.data();

    m_bind_group_layout = wgpuDeviceCreateBindGroupLayout(Renderer::get().device(), &bind_group_desc);
    ERR_COND(m_bind_group_layout == nullptr, "BindGroupLayout is invalid");

    WGPUPipelineLayoutDescriptor pipeline_layout_desc = WGPU_PIPELINE_LAYOUT_DESCRIPTOR_INIT;
    pipeline_layout_desc.bindGroupLayouts = &m_bind_group_layout;
    pipeline_layout_desc.bindGroupLayoutCount = 1;

    m_pipeline_layout = wgpuDeviceCreatePipelineLayout(Renderer::get().device(), &pipeline_layout_desc);
    ERR_COND(m_bind_group_layout == nullptr, "PipelineLayout is invalid");
}
