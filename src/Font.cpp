#include "Font.hpp"
#include "Core/Containers/Array.hpp"
#include "Core/Containers/Vector.hpp"
#include "Render/Graph.hpp"
#include "Render/Renderer.hpp"
#include "Render/Shader.hpp"
#include "Render/Types.hpp"
#include "webgpu/webgpu.h"

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library g_lib;
static Ref<Mesh> g_mesh;
static Ref<Shader> g_shader;

Result<Ref<Font>> Font::create(const StringView& font_name, uint32_t font_size)
{
    uint32_t bmp_height = 0;
    uint32_t bmp_width = 0;
    HashMap<uint8_t, Vector<char>> data;

    FT_Face face;

    if (FT_New_Face(g_lib, font_name.data(), 0, &face) != 0)
    {
        return Error(ErrorKind::FileNotFound);
    }

    FT_Set_Pixel_Sizes(face, 0, font_size);

    Ref<Font> font = TRY(newref<Font>());

    for (uint8_t c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0)
        {
            return Error(ErrorKind::Unknown);
        }

        bmp_height = std::max(bmp_height, face->glyph->bitmap.rows);

        const Character character = {
            .size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            .bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            .offset = bmp_width,
            .advance = (uint32_t)face->glyph->advance.x,
        };

        TRY(font->m_characters.put(c, character));

        if (face->glyph->bitmap.width > 0)
        {
            Vector<char> char_data;
            EXPECT(char_data.resize(face->glyph->bitmap.width * face->glyph->bitmap.rows));
            auto glyph_buffer = face->glyph->bitmap.buffer;

            memcpy(char_data.data(), glyph_buffer, face->glyph->bitmap.width * face->glyph->bitmap.rows);
            TRY(data.put(c, char_data));
        }
        bmp_width += face->glyph->bitmap.width;
    }

    LocalVector<uint8_t> buffer;
    TRY(buffer.resize(bmp_height * bmp_width));

    uint32_t xpos = 0;

    for (size_t i = 0; i < 128; i++)
    {
        if (!font->m_characters.contains(i) || !data.contains(i))
        {
            continue;
        }

        const Font::Character character = font->m_characters.get(i).get();
        const Vector<char>& char_data = *data.get_ptr(i).get();

        const int width = character.size.x;
        const int height = character.size.y;

        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {
                const char byte = char_data.get_unchecked(i + j * width);
                buffer.get_unchecked((i + xpos) + j * bmp_width) = byte;
            }
        }
        xpos += width;
    }

    font->m_width = bmp_width;
    font->m_height = bmp_height;

    font->m_bitmap = TRY(Texture::create(bmp_width, bmp_height, WGPUTextureFormat_R8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding));
    font->m_bitmap->update(buffer);

    return font;
}

Font::~Font()
{
}

Result<void> Font::init_library()
{
    const FT_Error res = FT_Init_FreeType(&g_lib);

    if (res != 0)
    {
        return Error(ErrorKind::FileNotFound);
    }

    Array<uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};
    Array<glm::vec3, 4> vertices = {
        glm::vec3(-0.5, 0.0, -1.0),
        glm::vec3(0.5, 0.0, -1.0),
        glm::vec3(0.5, -1.0, -1.0),
        glm::vec3(-0.5, -1.0, -1.0)};
    Array<glm::vec3, 4> normals = {
        glm::vec3(),
        glm::vec3(),
        glm::vec3(),
        glm::vec3(),

    };
    Array<glm::vec2, 4> uvs = {
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };

    g_mesh = EXPECT(Mesh::create_from_data(View(indices).as_bytes(), vertices, normals, View(uvs).as_bytes(), WGPUIndexFormat_Uint16));

    g_shader = TRY(Shader::load("assets/shaders/font.wgsl"));
    g_shader->set_binding("env", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 0, BindingAccess::Read));
    g_shader->set_binding("data", Binding(BindingKind::UniformBuffer, WGPUShaderStage_Vertex, 0, 1, BindingAccess::Read));
    g_shader->set_binding("bitmap", Binding(BindingKind::Texture, WGPUShaderStage_Fragment, 0, 2, BindingAccess::Read, TextureDimension::D2D));
    g_shader->create_bind_group_layout();

    return Result<void>();
}

void Font::deinit_library()
{
    g_shader = nullptr;
    g_mesh = nullptr;

    FT_Done_FreeType(g_lib);
}

Text::Text(Ref<Font> font)
    : m_font(font), m_instance_buffer(nullptr), m_capacity(0), m_size(0)
{
    m_uniform.color = glm::vec4(0.0, 0.0, 0.0, 1.0);
    m_uniform.position = glm::vec3();
    m_uniform.scale = glm::vec2(0.1);

    m_uniform_buffer = EXPECT(Buffer::create(sizeof(Font::Uniform), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    Vector<InstanceAttribute> attribs;
    EXPECT(attribs.append(InstanceAttribute(offsetof(Font::Instance, bounds), WGPUVertexFormat_Float32x4)));
    EXPECT(attribs.append(InstanceAttribute(offsetof(Font::Instance, char_pos), WGPUVertexFormat_Float32x2)));
    EXPECT(attribs.append(InstanceAttribute(offsetof(Font::Instance, scale), WGPUVertexFormat_Float32x2)));

    m_material = EXPECT(Material::create(g_shader, MaterialFlagBits::Transparency | MaterialFlagBits::NoNormal | MaterialFlagBits::NoUV, WGPUCullMode_None, UVType::UV, Instance(attribs, sizeof(Font::Instance))));
    m_material->set_param("env", Renderer::get().get_env_2d());
    m_material->set_param("data", m_uniform_buffer);
    m_material->set_param("bitmap", font->get_bitmap());
}

Text::Text(size_t capacity, Ref<Font> font)
    : Text(font)
{
    m_uniform.color = glm::vec4(0.0, 0.0, 0.0, 1.0);
    m_uniform.position = glm::vec3();
    m_uniform.scale = glm::vec2(0.1);

    m_capacity = capacity;
    m_instance_buffer = EXPECT(Buffer::create(m_capacity * sizeof(Font::Instance), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
}

void Text::set(const String& text)
{
    float offset_x = 0;

    const size_t width = m_font->get_width();
    const size_t height = m_font->get_height();

    if (text.size() > m_capacity)
    {
        m_capacity = text.size();
        m_instance_buffer = EXPECT(Buffer::create(m_capacity * sizeof(Font::Instance), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
    }

    constexpr size_t batch_size = 32;
    Array<Font::Instance, batch_size> instances{};

    for (size_t i = 0; i < text.size(); i++)
    {
        const uint8_t c = text[i];
        const Font::Character ch = m_font->get_character(c).value_or(Font::Character{});

        const float offset = (float)ch.offset / (float)width;
        const float char_width = (float)ch.size.x / (float)width;
        const float char_height = (float)ch.size.y / (float)height;

        const float bx = (float)ch.bearing.x / (float)width;
        const float by = (float)(ch.size.y - ch.bearing.y) / (float)height;

        const float scale_x = (float)ch.size.x / (float)height;
        const float scale_y = (float)ch.size.y / (float)height;

        instances[i % batch_size] = {
            .bounds = glm::vec4(offset, offset + char_width, char_height, 0.0f),
            .char_pos = glm::vec3(bx + offset_x, by, 0.0),
            .scale = glm::vec2(scale_x, scale_y),
        };

        if (i % batch_size == batch_size - 1 || i == text.size() - 1)
        {
            const size_t size = i + batch_size < text.size() ? batch_size : i - (i / batch_size) * batch_size + 1;
            View<Font::Instance> span(instances.data(), size);
            m_instance_buffer->update(span.as_bytes(), batch_size * sizeof(Font::Instance) * (i / batch_size));
        }

        offset_x += float(ch.advance >> 6) / float(height);
    }

    m_size = text.size();
}

void Text::set_position(glm::vec3 position)
{
    m_uniform.position = position;
    update_uniform_buffer();
}

void Text::set_scale(glm::vec2 scale)
{
    m_uniform.scale = scale;
    update_uniform_buffer();
}

void Text::set_color(glm::vec4 color)
{
    m_uniform.color = color;
    update_uniform_buffer();
}

void Text::draw(const RenderPassNode& node)
{
    WGPURenderPassEncoder encoder = node.encoder();
    const Ref<Mesh>& mesh = Renderer::get().get_square_mesh();

    WGPURenderPipeline pipeline = Renderer::get().get_pipeline(m_material, node);
    wgpuRenderPassEncoderSetPipeline(encoder, pipeline);
    wgpuRenderPassEncoderSetBindGroup(encoder, 0, m_material->get_bind_group(), 0, nullptr);

    wgpuRenderPassEncoderSetIndexBuffer(encoder, mesh->get_buffer(Mesh::BufferKind::Index)->handle(), mesh->index_type(), 0, mesh->get_buffer(Mesh::BufferKind::Index)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 0, mesh->get_buffer(Mesh::BufferKind::Position)->handle(), 0, mesh->get_buffer(Mesh::BufferKind::Position)->size());
    wgpuRenderPassEncoderSetVertexBuffer(encoder, 1, m_instance_buffer->handle(), 0, m_instance_buffer->size());

    wgpuRenderPassEncoderDrawIndexed(encoder, mesh->vertex_count(), m_size, 0, 0, 0);
}

void Text::update_uniform_buffer()
{
    m_uniform_buffer->update(View<Font::Uniform>(m_uniform).as_bytes());
}
