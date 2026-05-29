#pragma once

#include "Core/Containers/HashMap.hpp"
#include "Core/Ref.hpp"
#include "Core/Result.hpp"
#include "Render/Renderer.hpp"

class Font : public Object
{
    CLASS(Font, Object);

public:
    static Result<Ref<Font>> create(const StringView& font_name, uint32_t font_size);

    ~Font();
    static Result<void> init_library();
    static void deinit_library();

    struct Character
    {
        glm::ivec2 size;
        glm::ivec2 bearing;
        uint32_t offset;
        uint32_t advance;
    };

    struct __attribute__((aligned(16))) Uniform
    {
        __attribute__((aligned(16))) glm::vec4 color = glm::vec4(0.0);
        __attribute__((aligned(16))) glm::vec3 position = glm::vec3(0.0);
        __attribute__((aligned(16))) glm::vec2 scale = glm::vec2(0.1);
    };

    struct Instance
    {
        glm::vec4 bounds;
        glm::vec2 char_pos;
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

    inline Option<Character> get_character(uint8_t c)
    {
        return m_characters.get(c);
    }

    inline Ref<Texture> get_bitmap() const
    {
        return m_bitmap;
    }

private:
    Ref<Texture> m_bitmap;
    Ref<Buffer> m_buffer;
    Ref<Mesh> m_mesh;
    HashMap<uint8_t, Character> m_characters;

    size_t m_width;
    size_t m_height;
};

class Text
{
public:
    Text(Ref<Font> font);
    Text(size_t capacity, Ref<Font> font);

    Text(const String& text, Ref<Font> font)
        : Text(text.size(), font)
    {
        set(text);
    }

    ~Text()
    {
    }

    void set(const String& text);

    void set_position(glm::vec3 position);
    void set_scale(glm::vec2 scale);
    void set_color(glm::vec4 color);

    void draw(const RenderPassNode& node);

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
