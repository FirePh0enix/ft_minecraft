#pragma once

#include "Core/Result.hpp"
#include "Core/Types.hpp"
#include "Render/Renderer.hpp"

class Font : public Object
{
    CLASS(Font, Object);

public:
    static Result<std::shared_ptr<Font>> create(std::string_view font_name, uint32_t font_size);

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

    struct GPU_ATTRIBUTE Uniform
    {
        GPU_ATTRIBUTE glm::vec4 color = glm::vec4(0.0);
        GPU_ATTRIBUTE glm::vec3 position = glm::vec3(0.0);
        GPU_ATTRIBUTE glm::vec2 scale = glm::vec2(0.1);
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
        return m_mesh.get();
    }

    inline Option<Character> get_character(uint8_t c)
    {
        auto iter = m_characters.find(c);
        if (iter == m_characters.end())
            return iter->second;
        return None;
    }

    inline std::shared_ptr<Texture> get_bitmap() const
    {
        return m_bitmap;
    }

private:
    std::shared_ptr<Texture> m_bitmap;
    std::shared_ptr<Buffer> m_buffer;
    std::shared_ptr<Mesh> m_mesh;
    std::map<uint8_t, Character> m_characters;

    size_t m_width;
    size_t m_height;
};

class Text
{
public:
    Text(std::shared_ptr<Font> font);
    Text(size_t capacity, std::shared_ptr<Font> font);

    Text(const std::string& text, std::shared_ptr<Font> font)
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

    void draw(const RenderPass& pass);

private:
    std::shared_ptr<Font> m_font;
    std::shared_ptr<Buffer> m_instance_buffer;
    size_t m_capacity;
    size_t m_size;
    std::shared_ptr<BindGroup> m_bg;
    Font::Uniform m_uniform;
    std::shared_ptr<Buffer> m_uniform_buffer;

    void update_uniform_buffer();
};
