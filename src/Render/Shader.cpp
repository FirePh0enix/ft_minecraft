#include "Render/Shader.hpp"
#include "Core/Assert.hpp"
#include "Core/Filesystem.hpp"
#include "Core/Hash.hpp"
#include "Render/Renderer.hpp"

Shader::~Shader()
{
}

Result<Ref<Shader>> Shader::load(const std::filesystem::path& path)
{
    String source_code = TRY(TRY(Filesystem::open_file(path)).read_to_string());

    Ref<Shader> shader = TRY(newref<Shader>());
    shader->m_source_code = source_code;
    shader->m_hash = hash_fnv32(source_code);
    shader->m_entry_point_names[WGPUShaderStage_Vertex] = "vertex_main";
    shader->m_entry_point_names[WGPUShaderStage_Fragment] = "fragment_main";

    return shader;
}

static WGPUTextureViewDimension convert_texture_view_dimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::D1D:
        return WGPUTextureViewDimension_1D;
    case TextureDimension::D2D:
        return WGPUTextureViewDimension_2D;
    case TextureDimension::D2DArray:
        return WGPUTextureViewDimension_2DArray;
    case TextureDimension::D3D:
        return WGPUTextureViewDimension_3D;
    case TextureDimension::Cube:
        return WGPUTextureViewDimension_Cube;
    case TextureDimension::CubeArray:
        return WGPUTextureViewDimension_CubeArray;
    }
    return {};
}

static LocalVector<WGPUBindGroupLayoutEntry> convert_bindings(Shader *shader)
{
    LocalVector<WGPUBindGroupLayoutEntry> entries;
    EXPECT(entries.reserve(shader->get_bindings().size()));

    for (const auto& binding_pair : shader->get_bindings())
    {
        const Binding& binding = binding_pair.second;

        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = binding.shader_stage;
            entry.texture.sampleType = WGPUTextureSampleType_Float;
            entry.texture.multisampled = false;
            entry.texture.viewDimension = convert_texture_view_dimension(binding.dimension);

            EXPECT(entries.append(entry));

            WGPUBindGroupLayoutEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.visibility = binding.shader_stage;
            sampler_entry.sampler.nextInChain = nullptr;
            sampler_entry.sampler.type = WGPUSamplerBindingType_Filtering;

            EXPECT(entries.append(sampler_entry));
        }
        break;
        case BindingKind::UniformBuffer:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = binding.shader_stage;
            entry.buffer.type = WGPUBufferBindingType_Uniform;

            EXPECT(entries.append(entry));
        }
        break;
        case BindingKind::StorageBuffer:
        {
            WGPUBindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = binding.shader_stage;
            entry.buffer.type = binding.access == BindingAccess::Read ? WGPUBufferBindingType_ReadOnlyStorage : WGPUBufferBindingType_Storage;

            EXPECT(entries.append(entry));
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
    LocalVector<WGPUBindGroupLayoutEntry> entries = convert_bindings(this);

    // println("{} |<>| {}", entries.size(), entries.references());

    WGPUBindGroupLayoutDescriptor bind_group_desc{};
    bind_group_desc.entryCount = entries.size();
    bind_group_desc.entries = entries.data();

    m_bind_group_layout = wgpuDeviceCreateBindGroupLayout(Renderer::get().device(), &bind_group_desc);
    ERR_COND(m_bind_group_layout == nullptr, "BindGroupLayout is invalid");

    WGPUPipelineLayoutDescriptor pipeline_layout_desc{};
    pipeline_layout_desc.nextInChain = nullptr;
    pipeline_layout_desc.label = WGPU_STRING_VIEW_INIT;
    pipeline_layout_desc.bindGroupLayouts = &m_bind_group_layout;
    pipeline_layout_desc.bindGroupLayoutCount = 1;

    m_pipeline_layout = wgpuDeviceCreatePipelineLayout(Renderer::get().device(), &pipeline_layout_desc);
    ERR_COND(m_bind_group_layout == nullptr, "PipelineLayout is invalid");
}
