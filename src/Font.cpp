#include "Font.hpp"
#include "Core/Filesystem.hpp"
#include "Render/Graph.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

static FT_Library g_lib;
static glm::mat4 g_ortho_matrix;
static Ref<Mesh> g_mesh = nullptr;
static Ref<Shader> g_shader = nullptr;

Result<Ref<Font>> Font::create(const std::string& font_name, uint32_t font_size)
{
    uint32_t bmp_height = 0;
    uint32_t bmp_width = 0;
    std::map<uint8_t, std::vector<char>> data;

    FT_Face face;

    std::vector<char> file_bytes = Filesystem::read_file_to_buffer(font_name).value();

    if (FT_New_Memory_Face(g_lib, (FT_Byte *)file_bytes.data(), (int64_t)file_bytes.size(), 0, &face) != 0)
    {
        return Error(ErrorKind::FileNotFound);
    }

    FT_Set_Pixel_Sizes(face, 0, font_size);

    Ref<Font> font = make_ref<Font>();

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
        const std::vector<char> char_data = data[i];

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

    auto texture_result = RenderingDriver::get()->create_texture(bmp_width, bmp_height, TextureFormat::R8Unorm, TextureUsageFlagBits::CopyDest | TextureUsageFlagBits::Sampled);
    YEET(texture_result);
    font->m_bitmap = texture_result.value();

    font->m_bitmap->update(buffer);

    return font;
}

Font::~Font()
{
}

Result<> Font::init_library()
{
    const FT_Error res = FT_Init_FreeType(&g_lib);

    if (res != 0)
    {
        return Error(ErrorKind::FileNotFound);
    }

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

    View<uint16_t> indices_span = indices;

    g_mesh = Mesh::create_from_data(indices_span.as_bytes(), vertices, normals, uvs, IndexType::Uint16);

    const Extent2D size = RenderingDriver::get()->get_surface_extent();
    const float aspect_radio = (float)size.width / (float)size.height;

    g_ortho_matrix = glm::ortho(-1.0 * aspect_radio, 1.0 * aspect_radio, -1.0, 1.0, 0.01, 10.0);
#ifdef __platform_web
    // FIXME: For some reason this is needed on web and not desktop.
    g_ortho_matrix[1][1] *= -1;
#endif

    std::array<InstanceLayoutInput, 3> inputs = {InstanceLayoutInput(ShaderType::Float32x4, 0),
                                                 InstanceLayoutInput(ShaderType::Float32x3, sizeof(float) * 4),
                                                 InstanceLayoutInput(ShaderType::Float32x2, sizeof(float) * 7)};
    InstanceLayout instance_layout(inputs, sizeof(Instance));

    auto shader_result = Shader::load("assets/shaders/font.slang");
    if (!shader_result.has_value())
    {
        return Error(ErrorKind::ShaderCompilationFailed);
    }

    g_shader = shader_result.value();

    return 0;
}

void Font::deinit_library()
{
    g_shader = nullptr;
    g_mesh = nullptr;

    FT_Done_FreeType(g_lib);
}

Text::Text(size_t capacity, Ref<Font> font)
    : m_font(font), m_instance_buffer(nullptr), m_capacity(capacity), m_size(0)
{
    auto buffer_result = RenderingDriver::get()->create_buffer(nullptr, m_capacity, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
    ERR_EXPECT_R(buffer_result, "Cannot create the instance buffer");
    m_instance_buffer = buffer_result.value();

    auto font_uniform = RenderingDriver::get()->create_buffer(nullptr, 1, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Uniform);
    ERR_EXPECT_R(buffer_result, "Cannot create the uniform buffer");
    m_uniform_buffer = font_uniform.value();

    std::array<InstanceLayoutInput, 3> inputs = {InstanceLayoutInput(ShaderType::Float32x4, 0),
                                                 InstanceLayoutInput(ShaderType::Float32x3, sizeof(float) * 4),
                                                 InstanceLayoutInput(ShaderType::Float32x2, sizeof(float) * 7)};
    InstanceLayout instance_layout(inputs, sizeof(Font::Instance));

    m_material = Material::create(g_shader, sizeof(glm::mat4), instance_layout, MaterialFlagBits::Transparency);
    m_material->set_param("bitmap", font->get_bitmap());
    m_material->set_param("data", m_uniform_buffer);
}

void Text::set(const std::string& text)
{
    ZoneScoped;

    float offset_x = 0;

    const size_t width = m_font->get_width();
    const size_t height = m_font->get_height();

    if (text.length() > m_capacity)
    {
        m_capacity = text.length();
        auto instance_buffer_result = RenderingDriver::get()->create_buffer(nullptr, m_capacity, BufferUsageFlagBits::CopyDest | BufferUsageFlagBits::Vertex);
        ERR_EXPECT_R(instance_buffer_result, "Cannot create the instance buffer");
        m_instance_buffer = instance_buffer_result.value();
    }

    constexpr size_t batch_size = 32;
    std::array<Font::Instance, batch_size> instances{};

    for (size_t i = 0; i < text.length(); i++)
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

        instances[i % 32] = {
            .bounds = glm::vec4(offset, offset + char_width, char_height, 0.0f),
            .char_pos = glm::vec3(bx + offset_x, by, 0.0),
            .scale = glm::vec2(scale_x, scale_y),
        };

        if (i % batch_size == batch_size - 1 || i == text.size() - 1)
        {
            const size_t size = i + batch_size < text.size() ? batch_size : i - (i / batch_size) * batch_size + 1;
            View<Font::Instance> span(instances.data(), size);
            m_instance_buffer->update(span.as_bytes(), batch_size * (i / batch_size));
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

void Text::encode_draw_calls(RenderGraph& graph)
{
    // graph.add_draw(g_mesh, m_material, g_ortho_matrix, m_size, m_instance_buffer);
    (void)graph;
}

void Text::update_uniform_buffer()
{
    m_uniform_buffer->update(View<Font::Uniform>(m_uniform).as_bytes());
}
