#include "Font.hpp"
#include "Engine.hpp"
#include "Render/Renderer.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <memory>

static FT_Library g_lib;
static std::shared_ptr<Mesh> g_mesh;

Result<std::shared_ptr<Font>> Font::create(std::string_view font_name, uint32_t font_size)
{
    uint32_t bmp_height = 0;
    uint32_t bmp_width = 0;
    std::map<uint8_t, std::vector<char>> data;

    FT_Face face;

    if (FT_New_Face(g_lib, font_name.data(), 0, &face) != 0)
    {
        return Error(ErrorKind::FileNotFound);
    }

    FT_Set_Pixel_Sizes(face, 0, font_size);

    std::shared_ptr<Font> font = std::make_shared<Font>();

    for (uint8_t c = 0; c < 128; c++)
    {
        if (FT_Load_Char(face, c, FT_LOAD_RENDER) != 0)
            return Error(ErrorKind::Unknown);

        bmp_height = std::max(bmp_height, face->glyph->bitmap.rows);

        const Character character = {
            .size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            .bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            .offset = bmp_width,
            .advance = (uint32_t)face->glyph->advance.x,
        };

        font->m_characters[c] = character;

        if (face->glyph->bitmap.width > 0)
        {
            std::vector<char> char_data;
            char_data.resize(face->glyph->bitmap.width * face->glyph->bitmap.rows);
            auto glyph_buffer = face->glyph->bitmap.buffer;

            memcpy(char_data.data(), glyph_buffer, face->glyph->bitmap.width * face->glyph->bitmap.rows);
            data[c] = char_data;
        }
        bmp_width += face->glyph->bitmap.width;
    }

    std::vector<uint8_t> buffer;
    buffer.resize(bmp_height * bmp_width);

    uint32_t xpos = 0;

    for (size_t i = 0; i < 128; i++)
    {
        if (!font->m_characters.contains(i) || !data.contains(i))
        {
            continue;
        }

        const Font::Character character = font->m_characters[i];
        const std::vector<char>& char_data = data[i];

        const int width = character.size.x;
        const int height = character.size.y;

        for (int i = 0; i < width; i++)
        {
            for (int j = 0; j < height; j++)
            {
                const char byte = char_data[i + j * width];
                buffer[(i + xpos) + j * bmp_width] = byte;
            }
        }
        xpos += width;
    }

    font->m_width = bmp_width;
    font->m_height = bmp_height;

    font->m_bitmap = TRY(Texture::create(bmp_width, bmp_height, WGPUTextureFormat_R8Unorm, WGPUTextureUsage_CopyDst | WGPUTextureUsage_TextureBinding));
    font->m_bitmap->update(std::as_bytes(std::span(buffer)));

    return font;
}

Font::~Font()
{
}

Result<void> Font::init_library()
{
    const FT_Error res = FT_Init_FreeType(&g_lib);

    if (res != 0)
        return Error(ErrorKind::FileNotFound);

    std::array<uint16_t, 6> indices = {0, 1, 2, 0, 2, 3};
    std::array<glm::vec3, 4> vertices = {
        glm::vec3(-0.5, 0.0, -1.0),
        glm::vec3(0.5, 0.0, -1.0),
        glm::vec3(0.5, -1.0, -1.0),
        glm::vec3(-0.5, -1.0, -1.0)};
    std::array<glm::vec3, 4> normals = {
        glm::vec3(),
        glm::vec3(),
        glm::vec3(),
        glm::vec3(),

    };
    std::array<glm::vec2, 4> uvs = {
        glm::vec2(0.0, 0.0),
        glm::vec2(1.0, 0.0),
        glm::vec2(1.0, 1.0),
        glm::vec2(0.0, 1.0),
    };

    g_mesh = EXPECT(Mesh::create_from_data(std::as_bytes(std::span(indices)), vertices, normals, std::as_bytes(std::span(uvs)), WGPUIndexFormat_Uint16));
    return Result<void>();
}

void Font::deinit_library()
{
    g_mesh = nullptr;
    FT_Done_FreeType(g_lib);
}

Text::Text(std::shared_ptr<Font> font)
    : m_font(font), m_instance_buffer(nullptr), m_capacity(0), m_size(0), m_width_x(0)
{
    m_uniform.color = glm::vec4(0.0, 0.0, 0.0, 1.0);
    m_uniform.position = glm::vec3();
    m_uniform.scale = glm::vec2(0.1);

    m_uniform_buffer = EXPECT(Buffer::create(sizeof(Font::Uniform), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));

    m_bg = BindGroup::create(Renderer::get().get_fw_text_shader());
    m_bg->set_param("env", Renderer::get().get_env_2d());
    m_bg->set_param("uniforms", m_uniform_buffer);
    m_bg->set_param("bitmap", EXPECT(font->get_bitmap()->get_view(WGPUTextureViewDimension_2D)));
}

Text::Text(size_t capacity, std::shared_ptr<Font> font)
    : Text(font)
{
    m_uniform.color = glm::vec4(0.0, 0.0, 0.0, 1.0);
    m_uniform.position = glm::vec3();
    m_uniform.scale = glm::vec2(0.1);

    m_capacity = capacity;
    m_instance_buffer = EXPECT(Buffer::create(m_capacity * sizeof(Font::Instance), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
}

void Text::set(std::string_view text)
{
    m_width_x = 0;

    const size_t width = m_font->get_width();
    const size_t height = m_font->get_height();

    if (text.size() > m_capacity)
    {
        m_capacity = text.size();
        m_instance_buffer = EXPECT(Buffer::create(m_capacity * sizeof(Font::Instance), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex));
    }

    const Extent2D window_size = Engine::get().window()->size();

    constexpr size_t batch_size = 32;
    Font::Instance instances[batch_size]{};

    for (size_t i = 0; i < text.size(); i++)
    {
        const uint8_t c = text[i];
        const Font::Character ch = m_font->get_character(c).value_or(Font::Character{});

        instances[i % batch_size] = {
            .bounds = glm::vec4(float(ch.offset) / float(width),
                                float(ch.offset + ch.size.x) / float(width),
                                float(ch.size.y) / float(height),
                                0.0f),
            .char_pos = glm::vec3(float(ch.bearing.x + m_width_x) / float(window_size.width - 1),
                                  float(-ch.bearing.y) / float(window_size.width - 1),
                                  0.1),
            .scale = glm::vec2(float(ch.size.x) / float(window_size.width),
                               float(ch.size.y) / float(window_size.width)),
        };

        if (i % batch_size == batch_size - 1 || i == text.size() - 1)
        {
            const size_t size = i + batch_size < text.size() ? batch_size : i - (i / batch_size) * batch_size + 1;
            m_instance_buffer->update(std::as_bytes(std::span<Font::Instance>(instances, size)), batch_size * sizeof(Font::Instance) * (i / batch_size));
        }

        m_width_x += int32_t(ch.advance >> 6);
    }

    m_size = text.size();
    m_text = text;
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

int32_t Text::get_width() const
{
    return m_width_x;
}

int32_t Text::get_height() const
{
    return (int32_t)m_font->get_height();
}

void Text::draw(const RenderPass& pass)
{
    if (m_size > 0)
        Renderer::get().draw(pass, g_mesh, Renderer::get().get_fw_text_mat(), m_bg, m_instance_buffer, m_size);
}

void Text::update_uniform_buffer()
{
    m_uniform_buffer->update_struct(m_uniform);
}
