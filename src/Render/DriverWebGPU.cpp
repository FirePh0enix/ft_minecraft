#include "Render/DriverWebGPU.hpp"

#ifndef __platform_web
#error "WebGPU is only implemented on emscripten"
#endif

#ifdef __platform_web
#include <emscripten/html5.h>
#endif

#include <fstream>

static wgpu::TextureFormat convert_texture_format(TextureFormat format)
{
    switch (format)
    {
    case TextureFormat::R8Unorm:
        return wgpu::TextureFormat::R8Unorm;
    case TextureFormat::RGBA8Unorm:
        return wgpu::TextureFormat::RGBA8Unorm;
    case TextureFormat::RGBA8Srgb:
        return wgpu::TextureFormat::RGBA8UnormSrgb;
    case TextureFormat::BGRA8Srgb:
        return wgpu::TextureFormat::BGRA8UnormSrgb;
    case TextureFormat::R32Sfloat:
        return wgpu::TextureFormat::R32Float;
    case TextureFormat::RG32Sfloat:
        return wgpu::TextureFormat::RG32Float;
    case TextureFormat::RGBA32Sfloat:
        return wgpu::TextureFormat::RGBA32Float;
    case TextureFormat::D32:
        return wgpu::TextureFormat::Depth32Float;
    default:
        // RGB32Sfloat not supported
        std::abort();
        break;
    }
    return {};
}

static wgpu::VertexFormat convert_shader_type(ShaderType type)
{
    switch (type)
    {
    case ShaderType::Float:
        return wgpu::VertexFormat::Float32;
    case ShaderType::Vec2:
        return wgpu::VertexFormat::Float32x2;
    case ShaderType::Vec3:
        return wgpu::VertexFormat::Float32x3;
    case ShaderType::Vec4:
        return wgpu::VertexFormat::Float32x4;
    case ShaderType::Uint:
        return wgpu::VertexFormat::Uint32;
    }

    return {};
}

static wgpu::BufferUsage convert_buffer_usage(BufferUsage usage)
{
    wgpu::BufferUsage flags = {};

    if (usage.copy_src)
        flags |= wgpu::BufferUsage::CopySrc;
    if (usage.copy_dst)
        flags |= wgpu::BufferUsage::CopyDst;
    if (usage.uniform)
        flags |= wgpu::BufferUsage::Uniform;
    if (usage.index)
        flags |= wgpu::BufferUsage::Index;
    if (usage.vertex)
        flags |= wgpu::BufferUsage::Vertex;

    return flags;
}

static wgpu::TextureUsage convert_texture_usage(TextureUsage usage)
{
    wgpu::TextureUsage flags = {};

    if (usage.color_attachment)
        flags |= wgpu::TextureUsage::RenderAttachment;
    if (usage.depth_attachment)
        flags |= wgpu::TextureUsage::RenderAttachment;
    if (usage.copy_src)
        flags |= wgpu::TextureUsage::CopySrc;
    if (usage.copy_dst)
        flags |= wgpu::TextureUsage::CopyDst;
    if (usage.sampled)
        flags |= wgpu::TextureUsage::TextureBinding;

    return flags;
}

static wgpu::IndexFormat convert_index_type(IndexType type)
{
    switch (type)
    {
    case IndexType::Uint16:
        return wgpu::IndexFormat::Uint16;
    case IndexType::Uint32:
        return wgpu::IndexFormat::Uint32;
    }

    return {};
}

static wgpu::ShaderStage convert_shader_stage(ShaderStageKind kind)
{
    switch (kind)
    {
    case ShaderStageKind::Vertex:
        return wgpu::ShaderStage::Vertex;
    case ShaderStageKind::Fragment:
        return wgpu::ShaderStage::Fragment;
    case ShaderStageKind::Compute:
        return wgpu::ShaderStage::Compute;
    }

    return {};
}

static wgpu::CullMode convert_cull_mode(CullMode cull_mode)
{
    switch (cull_mode)
    {
    case CullMode::None:
        return wgpu::CullMode::None;
    case CullMode::Back:
        return wgpu::CullMode::Back;
    case CullMode::Front:
        return wgpu::CullMode::Front;
    };

    return {};
}

static wgpu::AddressMode convert_address_mode(AddressMode mode)
{
    switch (mode)
    {
    case AddressMode::ClampToEdge:
        return wgpu::AddressMode::ClampToEdge;
    case AddressMode::Repeat:
        return wgpu::AddressMode::Repeat;
    }

    return {};
}

static wgpu::FilterMode convert_filter_mode(Filter filter)
{
    switch (filter)
    {
    case Filter::Linear:
        return wgpu::FilterMode::Linear;
    case Filter::Nearest:
        return wgpu::FilterMode::Nearest;
    }

    return {};
}

static wgpu::TextureViewDimension convert_texture_dimension(TextureDimension dimension)
{
    switch (dimension)
    {
    case TextureDimension::D1D:
        return wgpu::TextureViewDimension::e1D;
    case TextureDimension::D2D:
        return wgpu::TextureViewDimension::e2D;
    case TextureDimension::D2DArray:
        return wgpu::TextureViewDimension::e2DArray;
    case TextureDimension::D3D:
        return wgpu::TextureViewDimension::e3D;
    case TextureDimension::Cube:
        return wgpu::TextureViewDimension::Cube;
    case TextureDimension::CubeArray:
        return wgpu::TextureViewDimension::CubeArray;
    }

    return {};
}

static wgpu::SamplerDescriptor convert_sampler(Sampler sampler)
{
    wgpu::SamplerDescriptor desc{};
    desc.magFilter = convert_filter_mode(sampler.mag_filter);
    desc.minFilter = convert_filter_mode(sampler.min_filter);
    desc.addressModeU = convert_address_mode(sampler.address_mode.u);
    desc.addressModeV = convert_address_mode(sampler.address_mode.v);
    desc.addressModeW = convert_address_mode(sampler.address_mode.w);

    return desc;
}

PipelineCache::PipelineCache()
{
}

Expected<wgpu::RenderPipeline> PipelineCache::get_or_create(Material *material)
{
    auto iter = m_pipelines.find({.material = material});

    if (iter != m_pipelines.end())
    {
        return iter->second;
    }
    else
    {
        MaterialWebGPU *material_wgpu = (MaterialWebGPU *)material;
        Ref<MaterialLayoutWebGPU> layout = material_wgpu->get_layout().cast_to<MaterialLayoutWebGPU>();

        auto pipeline_result = RenderingDriverWebGPU::get()->create_render_pipeline(layout->shader, layout->instance_layout, layout->cull_mode, layout->flags, layout->pipeline_layout);
        YEET(pipeline_result);

        m_pipelines[{.material = material}] = pipeline_result.value();
        return pipeline_result.value();
    }
}

SamplerCache::SamplerCache()
{
}

Expected<wgpu::Sampler> SamplerCache::get_or_create(Sampler sampler)
{
    auto iter = m_samplers.find(sampler);

    if (iter != m_samplers.end())
    {
        return iter->second;
    }
    else
    {
        wgpu::SamplerDescriptor desc = convert_sampler(sampler);

        wgpu::Sampler sampler_wgpu = RenderingDriverWebGPU::get()->get_device().CreateSampler(&desc);
        if (!sampler_wgpu)
            return std::unexpected(ErrorKind::BadDriver);

        m_samplers[sampler] = sampler_wgpu;
        return sampler_wgpu;
    }
}

RenderingDriverWebGPU::RenderingDriverWebGPU()
{
}

RenderingDriverWebGPU::~RenderingDriverWebGPU()
{
}

Expected<void> RenderingDriverWebGPU::initialize(const Window& window)
{
    m_instance = wgpu::CreateInstance(nullptr);

#ifdef __platform_web
    // On the web we use glue code to acquire a WGPUDevice.
    m_device = wgpu::Device(emscripten_webgpu_get_device());
    if (!m_device)
        return std::unexpected(ErrorKind::BadDriver);
#endif

    m_queue = m_device.GetQueue();
    if (!m_queue)
        return std::unexpected(ErrorKind::BadDriver);

    wgpu::SurfaceDescriptor surface_desc{};

#ifdef __platform_web
    wgpu::SurfaceDescriptorFromCanvasHTMLSelector canvas_selector{};
    canvas_selector.selector = "#canvas";
    surface_desc.nextInChain = &canvas_selector;
#endif

    m_surface = m_instance.CreateSurface(&surface_desc);
    m_surface_format = convert_texture_format(TextureFormat::RGBA8Unorm);

    // Create a bind group for push constants since WebGPU does not support them.
    wgpu::BindGroupLayoutEntry bind_group_entry{};
    bind_group_entry.binding = 0;
    bind_group_entry.buffer.type = wgpu::BufferBindingType::Uniform;
    bind_group_entry.visibility = wgpu::ShaderStage::Vertex;

    wgpu::BindGroupLayoutDescriptor push_constant_desc{};
    push_constant_desc.entryCount = 1;
    push_constant_desc.entries = &bind_group_entry;

    m_push_constant_layout = m_device.CreateBindGroupLayout(&push_constant_desc);
    if (!m_push_constant_layout)
        return std::unexpected(ErrorKind::BadDriver);

    YEET(configure_surface(window, VSync::On));

    return {};
}

Expected<void> RenderingDriverWebGPU::configure_surface(const Window& window, VSync vsync)
{
    Extent2D surface_extent = window.size();

    wgpu::SurfaceConfiguration config{};
    config.device = m_device;
    config.format = m_surface_format;
    config.width = surface_extent.width;
    config.height = surface_extent.height;
    config.presentMode = vsync == VSync::On ? wgpu::PresentMode::Fifo : wgpu::PresentMode::Immediate;

    auto depth_texture_result = create_texture(surface_extent.width, surface_extent.height, TextureFormat::D32, {.depth_attachment = true});
    YEET(depth_texture_result);

    Ref<TextureWebGPU> depth_texture = depth_texture_result.value().cast_to<TextureWebGPU>();

    m_surface.Configure(&config);
    m_surface_extent = surface_extent;
    m_depth_texture = depth_texture;

    return {};
}

void RenderingDriverWebGPU::poll()
{
}

void RenderingDriverWebGPU::limit_frames(uint32_t limit)
{
    (void)limit;
    // TODO
}

Expected<Ref<Buffer>> RenderingDriverWebGPU::create_buffer(size_t size, BufferUsage usage, BufferVisibility visibility)
{
    (void)visibility;

    wgpu::BufferDescriptor desc{};
    desc.size = size;
    desc.mappedAtCreation = false;
    desc.usage = convert_buffer_usage(usage);

    // WebGPU requires the size of an uniform buffer to a multiple of 16 bytes.
    if (usage.uniform && size % 16 > 0)
    {
        desc.size = (((size - 1) / 16) + 1) * 16;
    }

    wgpu::Buffer buffer = m_device.CreateBuffer(&desc);
    if (!buffer)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    return make_ref<BufferWebGPU>(buffer, desc.size).cast_to<Buffer>();
}

Expected<Ref<Texture>> RenderingDriverWebGPU::create_texture(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage)
{
    wgpu::TextureFormat format_wgpu = convert_texture_format(format);

    wgpu::TextureDescriptor desc{};
    desc.usage = convert_texture_usage(usage);
    desc.dimension = wgpu::TextureDimension::e2D;
    desc.size = wgpu::Extent3D(width, height, 1);
    desc.format = format_wgpu;
    desc.viewFormatCount = 1;
    desc.viewFormats = &format_wgpu;

    wgpu::Texture texture = m_device.CreateTexture(&desc);
    if (!texture)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    wgpu::TextureViewDescriptor view_desc{};
    view_desc.format = format_wgpu;
    view_desc.dimension = wgpu::TextureViewDimension::e2D;

    wgpu::TextureView view = texture.CreateView(&view_desc);
    if (!view)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    return make_ref<TextureWebGPU>(texture, view, width, height, format).cast_to<Texture>();
}

Expected<Ref<Texture>> RenderingDriverWebGPU::create_texture_array(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage, uint32_t layers)
{
    wgpu::TextureFormat format_wgpu = convert_texture_format(format);

    wgpu::TextureDescriptor desc{};
    desc.usage = convert_texture_usage(usage);
    desc.dimension = wgpu::TextureDimension::e2D;
    desc.size = wgpu::Extent3D(width, height, layers);
    desc.format = format_wgpu;
    desc.viewFormatCount = 1;
    desc.viewFormats = &format_wgpu;

    wgpu::Texture texture = m_device.CreateTexture(&desc);
    if (!texture)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    wgpu::TextureViewDescriptor view_desc{};
    view_desc.format = format_wgpu;
    view_desc.dimension = wgpu::TextureViewDimension::e2DArray;

    wgpu::TextureView view = texture.CreateView(&view_desc);
    if (!view)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    return make_ref<TextureWebGPU>(texture, view, width, height, format).cast_to<Texture>();
}

Expected<Ref<Texture>> RenderingDriverWebGPU::create_texture_cube(uint32_t width, uint32_t height, TextureFormat format, TextureUsage usage)
{
    wgpu::TextureFormat format_wgpu = convert_texture_format(format);

    wgpu::TextureDescriptor desc{};
    desc.usage = convert_texture_usage(usage);
    desc.dimension = wgpu::TextureDimension::e2D;
    desc.size = wgpu::Extent3D(width, height, 1);
    desc.format = format_wgpu;
    desc.viewFormatCount = 1;
    desc.viewFormats = &format_wgpu;

    wgpu::Texture texture = m_device.CreateTexture(&desc);
    if (!texture)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    wgpu::TextureViewDescriptor view_desc{};
    view_desc.format = format_wgpu;
    view_desc.dimension = wgpu::TextureViewDimension::Cube;

    wgpu::TextureView view = texture.CreateView(&view_desc);
    if (!view)
        return std::unexpected(ErrorKind::OutOfDeviceMemory);

    return make_ref<TextureWebGPU>(texture, view, width, height, format).cast_to<Texture>();
}

Expected<Ref<Mesh>> RenderingDriverWebGPU::create_mesh(IndexType index_type, Span<uint8_t> indices, Span<glm::vec3> vertices, Span<glm::vec2> uvs, Span<glm::vec3> normals)
{
    const uint32_t vertex_count = indices.size() / size_of(index_type);

    auto index_buffer_result = create_buffer(indices.size(), {.copy_dst = 1, .index = 1});
    YEET(index_buffer_result);
    Ref<Buffer> index_buffer = index_buffer_result.value();

    auto vertex_buffer_result = create_buffer(vertices.size() * sizeof(glm::vec3), {.copy_dst = 1, .vertex = 1});
    YEET(vertex_buffer_result);
    Ref<Buffer> vertex_buffer = vertex_buffer_result.value();

    auto normal_buffer_result = create_buffer(normals.size() * sizeof(glm::vec3), {.copy_dst = 1, .vertex = 1});
    YEET(normal_buffer_result);
    Ref<Buffer> normal_buffer = normal_buffer_result.value();

    auto uv_buffer_result = create_buffer(uvs.size() * sizeof(glm::vec2), {.copy_dst = 1, .vertex = 1});
    YEET(uv_buffer_result);
    Ref<Buffer> uv_buffer = uv_buffer_result.value();

    index_buffer->update(indices.as_bytes());
    vertex_buffer->update(vertices.as_bytes());
    normal_buffer->update(normals.as_bytes());
    uv_buffer->update(uvs.as_bytes());

    return make_ref<MeshWebGPU>(index_type, vertex_count, index_buffer, vertex_buffer, normal_buffer, uv_buffer).cast_to<Mesh>();
}

void RenderingDriverWebGPU::draw_graph(const RenderGraph& graph)
{
    wgpu::SurfaceTexture frame;
    m_surface.GetCurrentTexture(&frame);

    wgpu::TextureView view = frame.texture.CreateView(); // TODO: destroy at the end

    wgpu::CommandEncoder command_encoder = m_device.CreateCommandEncoder();
    wgpu::RenderPassEncoder render_pass_encoder = nullptr;

    for (const auto& instruction : graph.get_instructions())
    {
        ZoneNamed(draw_graph_instruction, true);

        switch (instruction.kind)
        {
        case InstructionKind::BeginRenderPass:
        {
            wgpu::RenderPassColorAttachment color_attach{};
            color_attach.clearValue = wgpu::Color(0.0, 0.0, 0.0, 1.0);
            color_attach.loadOp = wgpu::LoadOp::Clear;
            color_attach.storeOp = wgpu::StoreOp::Store;
            color_attach.view = view;

            wgpu::RenderPassDepthStencilAttachment depth_attach{};
            depth_attach.depthClearValue = 1.0;
            depth_attach.depthLoadOp = wgpu::LoadOp::Clear;
            depth_attach.depthStoreOp = wgpu::StoreOp::Discard;
            depth_attach.view = m_depth_texture->view;

            wgpu::RenderPassDescriptor desc{};
            desc.colorAttachmentCount = 1;
            desc.colorAttachments = &color_attach;
            desc.depthStencilAttachment = &depth_attach;

            render_pass_encoder = command_encoder.BeginRenderPass(&desc);
        }
        break;
        case InstructionKind::EndRenderPass:
        {
            render_pass_encoder.End();
            render_pass_encoder = nullptr;
            break;
        }
        case InstructionKind::Draw:
        {
            MeshWebGPU *mesh = (MeshWebGPU *)instruction.draw.mesh;

            MaterialWebGPU *material = (MaterialWebGPU *)instruction.draw.material;
            const Ref<MaterialLayoutWebGPU>& material_layout = material->get_layout().cast_to<MaterialLayoutWebGPU>();

            std::optional<Buffer *> instance_buffer = instruction.draw.instance_buffer;

            auto pipeline_result = m_pipeline_cache.get_or_create(material);
            ERR_EXPECT_B(pipeline_result, "Failed to create a pipeline for the material");

            render_pass_encoder.SetPipeline(pipeline_result.value());
            render_pass_encoder.SetBindGroup(0, material->get_bind_group());

            Ref<BufferWebGPU> index_buffer = mesh->index_buffer.cast_to<BufferWebGPU>();
            Ref<BufferWebGPU> vertex_buffer = mesh->vertex_buffer.cast_to<BufferWebGPU>();
            Ref<BufferWebGPU> normal_buffer = mesh->normal_buffer.cast_to<BufferWebGPU>();
            Ref<BufferWebGPU> uv_buffer = mesh->uv_buffer.cast_to<BufferWebGPU>();

            render_pass_encoder.SetIndexBuffer(index_buffer->buffer, convert_index_type(mesh->index_type));
            render_pass_encoder.SetVertexBuffer(0, vertex_buffer->buffer);
            render_pass_encoder.SetVertexBuffer(1, normal_buffer->buffer);
            render_pass_encoder.SetVertexBuffer(2, uv_buffer->buffer);

            if (instance_buffer.has_value())
            {
                render_pass_encoder.SetVertexBuffer(3, ((BufferWebGPU *)instance_buffer.value())->buffer);
            }

            render_pass_encoder.SetViewport(0.0, 0.0, (float)m_surface_extent.width, (float)m_surface_extent.height, 0.0, 1.0);
            render_pass_encoder.SetScissorRect(0, 0, m_surface_extent.width, m_surface_extent.height);

            PushConstants push_constants{
                .view_matrix = instruction.draw.view_matrix,
            };

            m_queue.WriteBuffer(material->push_constant_buffer.cast_to<BufferWebGPU>()->buffer, 0, &push_constants, sizeof(PushConstants));

            render_pass_encoder.SetBindGroup(1, material->push_constant_bind_group);
            render_pass_encoder.DrawIndexed(mesh->vertex_count(), instruction.draw.instance_count);

            break;
        }
        case InstructionKind::Copy:
        {
            // TODO
            break;
        }
        }
    }

    wgpu::CommandBuffer cb = command_encoder.Finish();

    m_queue.Submit(1, &cb);

#ifdef __platform_web
    emscripten_request_animation_frame([](double, void *)
                                       { return true; }, nullptr);
#else
    m_surface.Present();
#endif
}

Expected<Ref<MaterialLayout>> RenderingDriverWebGPU::create_material_layout(Ref<Shader> shader, MaterialFlags flags, std::optional<InstanceLayout> instance_layout, CullMode cull_mode, PolygonMode polygon_mode)
{
    // Changing `polygon_mode` is not supported on WebGPU.
    (void)polygon_mode;

    std::vector<wgpu::BindGroupLayoutEntry> entries;
    entries.reserve(shader->get_bindings().size());

    for (const auto& binding_pair : shader->get_bindings())
    {
        const Binding& binding = binding_pair.second;

        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            wgpu::BindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = convert_shader_stage(binding.shader_stage);
            entry.texture.sampleType = wgpu::TextureSampleType::Float;
            entry.texture.multisampled = false;
            entry.texture.viewDimension = convert_texture_dimension(binding.dimension);

            entries.push_back(entry);

            wgpu::BindGroupLayoutEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.visibility = convert_shader_stage(binding.shader_stage);
            sampler_entry.sampler.type = wgpu::SamplerBindingType::Filtering;

            entries.push_back(sampler_entry);
        }
        break;
        case BindingKind::UniformBuffer:
        {
            wgpu::BindGroupLayoutEntry entry{};
            entry.binding = binding.binding;
            entry.visibility = convert_shader_stage(binding.shader_stage);
            entry.buffer.type = wgpu::BufferBindingType::Uniform;

            entries.push_back(entry);
        }
        break;
        }
    }

    wgpu::BindGroupLayoutDescriptor bind_group_layout_desc{};
    bind_group_layout_desc.entries = entries.data();
    bind_group_layout_desc.entryCount = entries.size();

    wgpu::BindGroupLayout bind_group_layout = m_device.CreateBindGroupLayout(&bind_group_layout_desc);
    if (!bind_group_layout)
        return std::unexpected(ErrorKind::BadDriver);

    std::array<wgpu::BindGroupLayout, 2> layouts{bind_group_layout, m_push_constant_layout};

    wgpu::PipelineLayoutDescriptor pipeline_layout_desc{};
    pipeline_layout_desc.bindGroupLayoutCount = layouts.size();
    pipeline_layout_desc.bindGroupLayouts = layouts.data();

    wgpu::PipelineLayout pipeline_layout = m_device.CreatePipelineLayout(&pipeline_layout_desc);
    if (!pipeline_layout)
        return std::unexpected(ErrorKind::BadDriver);

    return make_ref<MaterialLayoutWebGPU>(bind_group_layout, shader, instance_layout, convert_cull_mode(cull_mode), flags, pipeline_layout).cast_to<MaterialLayout>();
}

Expected<Ref<Material>> RenderingDriverWebGPU::create_material(MaterialLayout *layout)
{
    auto push_constant_buffer_result = create_buffer(sizeof(PushConstants), {.copy_dst = true, .uniform = true}, BufferVisibility::GPUAndCPU);
    Ref<Buffer> push_constant_buffer = push_constant_buffer_result.value();

    wgpu::BindGroupEntry push_constant_entry{};
    push_constant_entry.binding = 0;
    push_constant_entry.buffer = push_constant_buffer.cast_to<BufferWebGPU>()->buffer;

    wgpu::BindGroupDescriptor push_constant_desc{};
    push_constant_desc.layout = RenderingDriverWebGPU::get()->get_push_constant_layout();
    push_constant_desc.entryCount = 1;
    push_constant_desc.entries = &push_constant_entry;

    wgpu::BindGroup push_constant = m_device.CreateBindGroup(&push_constant_desc);
    if (!push_constant)
        return std::unexpected(ErrorKind::BadDriver);

    return make_ref<MaterialWebGPU>(layout, push_constant_buffer, push_constant).cast_to<Material>();
}

Expected<wgpu::RenderPipeline> RenderingDriverWebGPU::create_render_pipeline(Ref<Shader> shader, std::optional<InstanceLayout> instance_layout, wgpu::CullMode cull_mode, MaterialFlags flags, wgpu::PipelineLayout pipeline_layout)
{
    std::vector<wgpu::VertexBufferLayout> buffers;
    buffers.reserve(instance_layout.has_value() ? 4 : 3);

    wgpu::VertexAttribute vertex_attrib{};
    vertex_attrib.format = wgpu::VertexFormat::Float32x3;
    vertex_attrib.offset = 0;
    vertex_attrib.shaderLocation = 0;
    buffers.push_back(wgpu::VertexBufferLayout{.stepMode = wgpu::VertexStepMode::Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &vertex_attrib});

    wgpu::VertexAttribute normal_attrib{};
    normal_attrib.format = wgpu::VertexFormat::Float32x3;
    normal_attrib.offset = 0;
    normal_attrib.shaderLocation = 1;
    buffers.push_back(wgpu::VertexBufferLayout{.stepMode = wgpu::VertexStepMode::Vertex, .arrayStride = sizeof(glm::vec3), .attributeCount = 1, .attributes = &normal_attrib});

    wgpu::VertexAttribute uv_attrib{};
    uv_attrib.format = wgpu::VertexFormat::Float32x2;
    uv_attrib.offset = 0;
    uv_attrib.shaderLocation = 2;
    buffers.push_back(wgpu::VertexBufferLayout{.stepMode = wgpu::VertexStepMode::Vertex, .arrayStride = sizeof(glm::vec2), .attributeCount = 1, .attributes = &uv_attrib});

    std::vector<wgpu::VertexAttribute> instance_attribs;

    if (instance_layout)
    {
        InstanceLayout layout = instance_layout.value();
        instance_attribs.reserve(layout.inputs.size());

        for (size_t i = 0; i < layout.inputs.size(); i++)
        {
            const InstanceLayoutInput& input = layout.inputs[i];
            const wgpu::VertexFormat format = convert_shader_type(input.type);
            instance_attribs.push_back(wgpu::VertexAttribute{.format = format, .offset = input.offset, .shaderLocation = 3 + i});
        }

        buffers.push_back(wgpu::VertexBufferLayout{.stepMode = wgpu::VertexStepMode::Instance, .arrayStride = layout.stride, .attributeCount = instance_attribs.size(), .attributes = instance_attribs.data()});
    }

    std::string shader_code = shader->get_code();

    wgpu::ShaderModuleWGSLDescriptor wgsl_desc{};
    wgsl_desc.code = shader_code.c_str();
    wgpu::ShaderModuleDescriptor module_desc{};
    module_desc.nextInChain = &wgsl_desc;

    wgpu::ShaderModule module = m_device.CreateShaderModule(&module_desc);

    wgpu::VertexState vertex_state{};
    vertex_state.buffers = buffers.data();
    vertex_state.bufferCount = buffers.size();
    vertex_state.entryPoint = "vertex_main";
    vertex_state.module = module;

    wgpu::ColorTargetState color_state{};
    color_state.format = m_surface_format;

    wgpu::BlendState blend_state{};

    if (!flags.transparency)
    {
        blend_state.color.srcFactor = wgpu::BlendFactor::One;
        blend_state.color.dstFactor = wgpu::BlendFactor::Zero;
        blend_state.color.operation = wgpu::BlendOperation::Add;

        blend_state.alpha.srcFactor = wgpu::BlendFactor::One;
        blend_state.alpha.dstFactor = wgpu::BlendFactor::Zero;
        blend_state.alpha.operation = wgpu::BlendOperation::Add;
    }
    else
    {
        blend_state.color.srcFactor = wgpu::BlendFactor::SrcAlpha;
        blend_state.color.dstFactor = wgpu::BlendFactor::OneMinusSrcAlpha;
        blend_state.color.operation = wgpu::BlendOperation::Add;

        blend_state.alpha.srcFactor = wgpu::BlendFactor::One;
        blend_state.alpha.dstFactor = wgpu::BlendFactor::Zero;
        blend_state.alpha.operation = wgpu::BlendOperation::Add;
    }

    std::array<wgpu::ColorTargetState, 1> states;
    states[0] = wgpu::ColorTargetState{.format = m_surface_format, .blend = &blend_state};

    wgpu::FragmentState fragment_state{};
    fragment_state.targets = states.data();
    fragment_state.targetCount = 1;
    fragment_state.entryPoint = "fragment_main";
    fragment_state.module = module;

    wgpu::DepthStencilState depth_state{};
    depth_state.format = wgpu::TextureFormat::Depth32Float;
    depth_state.depthCompare = flags.always_first ? wgpu::CompareFunction::Less : wgpu::CompareFunction::LessEqual;
    depth_state.depthWriteEnabled = true;
    depth_state.depthCompare = wgpu::CompareFunction::Less;

    wgpu::PrimitiveState primitive_state{};
    primitive_state.cullMode = cull_mode;
    primitive_state.frontFace = wgpu::FrontFace::CCW;
    primitive_state.topology = wgpu::PrimitiveTopology::TriangleList;

    wgpu::RenderPipelineDescriptor desc{};
    desc.layout = pipeline_layout;
    desc.primitive = primitive_state;
    desc.vertex = vertex_state;
    desc.fragment = &fragment_state;
    desc.depthStencil = &depth_state;

    wgpu::RenderPipeline pipeline = m_device.CreateRenderPipeline(&desc);
    if (!pipeline)
        return std::unexpected(ErrorKind::BadDriver);

    return pipeline;
}

void BufferWebGPU::update(Span<uint8_t> view, size_t offset)
{
    RenderingDriverWebGPU::get()->get_queue().WriteBuffer(buffer, offset, view.data(), view.size());
}

void TextureWebGPU::update(Span<uint8_t> view, uint32_t layer)
{
    wgpu::ImageCopyTexture dest{};
    dest.texture = texture;
    dest.aspect = wgpu::TextureAspect::All;

    dest.origin.x = 0;
    dest.origin.y = 0;
    dest.origin.z = layer;

    wgpu::TextureDataLayout data_layout{};
    data_layout.bytesPerRow = size_of(format) * m_width;
    data_layout.rowsPerImage = m_height;
    data_layout.offset = 0;

    wgpu::Extent3D write_size(m_width, m_height, 1);

    RenderingDriverWebGPU::get()->get_queue().WriteTexture(&dest, view.data(), view.size(), &data_layout, &write_size);
}

void MaterialWebGPU::set_param(const std::string& name, const Ref<Texture>& texture)
{
    Ref<MaterialLayoutWebGPU> layout = m_layout.cast_to<MaterialLayoutWebGPU>();

    auto binding_result = layout->shader->get_binding(name);
    ERR_COND_V(!binding_result.has_value(), "Invalid parameter name `%s`", name.c_str());

    caches[name] = ParamCache{.texture = {.kind = BindingKind::Texture, .texture = texture.cast_to<TextureWebGPU>().ptr()}};
    dirty = true;
}

void MaterialWebGPU::set_param(const std::string& name, const Ref<Buffer>& buffer)
{
    Ref<MaterialLayoutWebGPU> layout = m_layout.cast_to<MaterialLayoutWebGPU>();

    auto binding_result = layout->shader->get_binding(name);
    ERR_COND_V(!binding_result.has_value(), "Invalid parameter name `%s`", name.c_str());

    caches[name] = ParamCache{.buffer = {.kind = BindingKind::Texture, .buffer = buffer.cast_to<BufferWebGPU>().ptr()}};
    dirty = true;
}

wgpu::BindGroup MaterialWebGPU::get_bind_group()
{
    if (!dirty)
    {
        return bind_group;
    }

    // TODO: Destroy the previous bind group

    MaterialLayoutWebGPU *layout_wgpu = (MaterialLayoutWebGPU *)m_layout.ptr();

    std::vector<wgpu::BindGroupEntry> entries;
    entries.reserve(layout_wgpu->shader->get_bindings().size());

    for (const auto& binding_pair : layout_wgpu->shader->get_bindings())
    {
        const Binding& binding = binding_pair.second;

        switch (binding.kind)
        {
        case BindingKind::Texture:
        {
            ParamCache cache = caches[binding_pair.first];

            wgpu::BindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.textureView = cache.texture.texture->view;

            entries.push_back(entry);

            Sampler sampler = layout_wgpu->shader->get_sampler(binding_pair.first);

            auto sampler_result = RenderingDriverWebGPU::get()->get_sampler_cache().get_or_create(sampler);
            ERR_EXPECT_B(sampler_result, "");

            wgpu::BindGroupEntry sampler_entry{};
            sampler_entry.binding = binding.binding + 1;
            sampler_entry.sampler = sampler_result.value();

            entries.push_back(sampler_entry);
        }
        break;
        case BindingKind::UniformBuffer:
        {
            ParamCache cache = caches[binding_pair.first];

            wgpu::BindGroupEntry entry{};
            entry.binding = binding.binding;
            entry.buffer = cache.buffer.buffer->buffer;

            entries.push_back(entry);
        }
        break;
        }
    }

    wgpu::BindGroupDescriptor desc{};
    desc.layout = layout_wgpu->layout;
    desc.entries = entries.data();
    desc.entryCount = entries.size();

    bind_group = RenderingDriverWebGPU::get()->get_device().CreateBindGroup(&desc);
    return bind_group;
}
