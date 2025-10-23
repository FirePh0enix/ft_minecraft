#include "Render/Driver.hpp"

size_t size_of(const TextureFormat& format)
{
    switch (format)
    {
    case TextureFormat::R8Unorm:
        return 1;
    case TextureFormat::R32Sfloat:
    case TextureFormat::RGBA8Srgb:
    case TextureFormat::RGBA8Unorm:
    case TextureFormat::BGRA8Srgb:
    case TextureFormat::Depth32:
        return 4;
    case TextureFormat::RG32Sfloat:
        return 8;
    case TextureFormat::RGB32Sfloat:
        return 12;
    case TextureFormat::RGBA32Sfloat:
        return 16;
    }

    return 0;
}

size_t size_of(const IndexType& format)
{
    switch (format)
    {
    case IndexType::Uint16:
        return 2;
    case IndexType::Uint32:
        return 4;
    };

    return 0;
}

Result<Ref<Buffer>> RenderingDriver::create_buffer_from_data(size_t size, View<uint8_t> data, BufferUsageFlags flags, BufferVisibility visibility)
{
    auto buffer_result = create_buffer(size, flags, visibility);
    YEET(buffer_result);

    Ref<Buffer> buffer = buffer_result.value();
    update_buffer(buffer, data, 0);

    return buffer;
}

Ref<Mesh> Mesh::create_from_data(const View<uint8_t>& indices, const View<glm::vec3>& positions, const View<glm::vec3>& normals, const View<glm::vec2>& uvs, IndexType index_type)
{
    const size_t vertex_count = indices.size() / size_of(index_type);

    auto index_buffer_result = RenderingDriver::get()->create_buffer(indices.size(), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Index);
    Ref<Buffer> index_buffer = index_buffer_result.value();

    auto vertex_buffer_result = RenderingDriver::get()->create_buffer(positions.size() * sizeof(glm::vec3), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    Ref<Buffer> vertex_buffer = vertex_buffer_result.value();

    auto normal_buffer_result = RenderingDriver::get()->create_buffer(normals.size() * sizeof(glm::vec3), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    Ref<Buffer> normal_buffer = normal_buffer_result.value();

    auto uv_buffer_result = RenderingDriver::get()->create_buffer(uvs.size() * sizeof(glm::vec2), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    Ref<Buffer> uv_buffer = uv_buffer_result.value();

    RenderingDriver::get()->update_buffer(index_buffer, indices.as_bytes(), 0);
    RenderingDriver::get()->update_buffer(vertex_buffer, positions.as_bytes(), 0);
    RenderingDriver::get()->update_buffer(normal_buffer, normals.as_bytes(), 0);
    RenderingDriver::get()->update_buffer(uv_buffer, uvs.as_bytes(), 0);

    return make_ref<Mesh>(vertex_count, index_type, index_buffer, vertex_buffer, normal_buffer, uv_buffer);
}

Ref<Material> Material::create(const Ref<Shader>& shader, size_t push_constant_size, std::optional<InstanceLayout> instance_layout, MaterialFlags flags, PolygonMode polygon_mode, CullMode cull_mode)
{
    return make_ref<Material>(shader, push_constant_size, instance_layout, flags, polygon_mode, cull_mode);
}

Ref<ComputeMaterial> ComputeMaterial::create(const Ref<Shader>& shader, size_t push_constant_size)
{
    return make_ref<ComputeMaterial>(shader, push_constant_size);
}

void MaterialBase::set_param(const std::string& name, const Ref<Texture>& texture)
{
    auto binding_result = get_shader()->get_binding(name);
    ERR_COND_V(!binding_result.has_value(), "Invalid parameter name `%s`", name.c_str());

    m_caches[name] = MaterialParamCache{.texture = {.kind = BindingKind::Texture, .texture = texture.ptr()}};
    m_param_changed = true;
}

void MaterialBase::set_param(const std::string& name, const Ref<Buffer>& buffer)
{
    auto binding_result = get_shader()->get_binding(name);
    ERR_COND_V(!binding_result.has_value(), "Invalid parameter name `%s`", name.c_str());

    m_caches[name] = MaterialParamCache{.buffer = {.kind = BindingKind::UniformBuffer, .buffer = buffer.ptr()}};
    m_param_changed = true;
}

const MaterialParamCache& MaterialBase::get_param(const std::string& name) const
{
    return m_caches.at(name);
}

void MaterialBase::clear_param_changed()
{
    m_param_changed = false;
}
