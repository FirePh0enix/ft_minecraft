#include "Render/Driver.hpp"
#include "Render/Graph.hpp"

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
    TRY(buffer_result);

    Ref<Buffer> buffer = buffer_result.value();
    buffer->update(data);

    return buffer;
}

Ref<Mesh> Mesh::create_from_data(const View<uint8_t>& indices, const View<glm::vec3>& positions, const View<glm::vec3>& normals, const View<uint8_t>& uvs, IndexType index_type, UVType uv_type)
{
    const size_t vertex_count = indices.size() / size_of(index_type);

    auto index_buffer_result = RenderingDriver::get()->create_buffer(indices.size(), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Index);
    Ref<Buffer> index_buffer = index_buffer_result.value();

    auto vertex_buffer_result = RenderingDriver::get()->create_buffer(positions.size() * sizeof(glm::vec3), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    Ref<Buffer> vertex_buffer = vertex_buffer_result.value();

    auto normal_buffer_result = RenderingDriver::get()->create_buffer(normals.size() * sizeof(glm::vec3), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    Ref<Buffer> normal_buffer = normal_buffer_result.value();

    auto uv_buffer_result = RenderingDriver::get()->create_buffer(uvs.size(), BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    Ref<Buffer> uv_buffer = uv_buffer_result.value();

    index_buffer->update(indices.as_bytes());
    vertex_buffer->update(positions.as_bytes());
    normal_buffer->update(normals.as_bytes());
    uv_buffer->update(uvs.as_bytes());

    return newobj(Mesh, vertex_count, index_type, uv_type, index_buffer, vertex_buffer, normal_buffer, uv_buffer);
}

void Material::set_param(const StringView& name, const Ref<Texture>& texture)
{
    auto binding_result = get_shader()->get_binding(name);
    ERR_COND_VR(texture.is_null(), "Texture specified for {} is null", name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name.data());

    // TODO: Check dimensions.

    m_caches[name] = MaterialParamCache{.texture = {.kind = BindingKind::Texture, .texture = texture.ptr()}};
    m_param_changed = true;
}

void Material::set_param(const StringView& name, const Ref<Buffer>& buffer)
{
    auto binding_result = get_shader()->get_binding(name);
    ERR_COND_VR(buffer.is_null(), "Buffer specified for {} is null", name);
    ERR_COND_VR(!binding_result.has_value(), "Invalid parameter name `{}`", name.data());

    m_caches[name] = MaterialParamCache{.buffer = {.kind = BindingKind::UniformBuffer, .buffer = buffer.ptr()}};
    m_param_changed = true;
}

const MaterialParamCache& Material::get_param(const StringView& name) const
{
    ASSERT_V(m_caches.contains(name), "Cache missing {}", name);
    return m_caches.at(name);
}

void Material::clear_param_changed()
{
    m_param_changed = false;
}
