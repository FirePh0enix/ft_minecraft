#pragma once

#include "Core/Error.hpp"
#include "Core/Ref.hpp"
#include "Render/Driver.hpp"

#include <map>

class Font
{
public:
    static Expected<Ref<Font>> create(const std::string& font_name, uint32_t font_size);

    ~Font();
    static Expected<void> init_library();
    static void deinit_library();

    struct Character
    {
        glm::ivec2 size;
        glm::ivec2 bearing;
        uint32_t offset;
        uint32_t advance;
    };

    struct Uniform
    {
        glm::vec4 color = glm::vec4(0.0, 0.0, 0.0, 1.0);
        glm::vec3 position = glm::vec3(0.0);
        glm::vec2 scale = glm::vec2(0.1);
    };

    struct Instance
    {
        glm::vec4 bounds;
        glm::vec3 char_pos;
        glm::vec2 scale;
    };

    inline size_t get_width() const
    {
        return m_width;
    }

    inline size_t get_height() const
    {
        return m_height;
    }

    inline Mesh *get_mesh() const
    {
        return m_mesh.ptr();
    }

    inline std::optional<Character> get_character(uint8_t c)
    {
        if (!m_characters.contains(c))
            return std::nullopt;
        return m_characters[c];
    }

    inline Ref<Texture> get_bitmap() const
    {
        return m_bitmap;
    }

private:
    Ref<Texture> m_bitmap;
    Ref<Buffer> m_buffer;
    Ref<Mesh> m_mesh;
    std::map<uint8_t, Character> m_characters;

    size_t m_width;
    size_t m_height;
};

class Text
{
public:
    Text()
        : m_font(nullptr), m_instance_buffer(nullptr), m_capacity(0)
    {
    }

    Text(size_t capacity, Ref<Font> font);

    Text(const std::string& text, Ref<Font> font)
        : Text(text.size(), font)
    {
        set(text);
    }

    ~Text()
    {
    }

    void set(const std::string& text);

    void set_position(glm::vec3 position);
    void set_scale(glm::vec2 scale);
    void set_color(glm::vec4 color);

    void encode_draw_calls(RenderGraph& graph);

private:
    Ref<Font> m_font;
    Ref<Buffer> m_instance_buffer;
    size_t m_capacity;
    size_t m_size;
    Ref<Material> m_material;
    Font::Uniform m_uniform;
    Ref<Buffer> m_uniform_buffer;

    void update_uniform_buffer();
};