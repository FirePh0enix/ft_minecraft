#include "Entity/Item.hpp"

#include "Core/Types.hpp"
#include "Engine.hpp"
#include "Render/Renderer.hpp"

struct GPU_ATTRIBUTE ItemBlockModel
{
    glm::mat4 model_matrix;
    glm::uvec3 textures;
};

ItemEntity::ItemEntity(Id<Item> item)
    : m_item(item), m_time(0)
{
    m_aabb = AABB(-glm::vec3(0.2, 0.2, 0.2), glm::vec3(0.2, 0.2, 0.2));
    get_transform().scale() = glm::vec3(0.2, 0.2, 0.2);

    m_model_buffer = EXPECT(Buffer::create(sizeof(ItemBlockModel), WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform));
    m_material = EXPECT(Material::create(Renderer::get().get_item_block_shader(), MaterialFlagBits::None, WGPUCullMode_Back, UVType::UV));
    m_material->set_param("env", Renderer::get().get_world_environment());
    m_material->set_param("model", m_model_buffer);
    m_material->set_param("images", Engine::get().registry().get_texture_array());

    Ref<Block> block = Engine::get().registry().block_from_item(item);
    m_textures = glm::uvec3(block->get_texture_ids()[0] | (block->get_texture_ids()[1] << 16), block->get_texture_ids()[2] | (block->get_texture_ids()[3] << 16), block->get_texture_ids()[4] | (block->get_texture_ids()[5] << 16));
}

void ItemEntity::tick(float delta)
{
    m_velocity.y -= m_gravity_value * delta;
    move_and_collide();
    m_time += delta;
}

void ItemEntity::draw(WGPURenderPassEncoder encoder)
{
    m_transform.rotation() = glm::rotate(glm::identity<glm::quat>(), m_time, glm::vec3(0.0, 1.0, 0.0));

    ItemBlockModel matrix(get_transform().to_matrix(), m_textures);

    m_model_buffer->update(View(matrix).as_bytes());
    Renderer::get().record_simple_shape(encoder, m_material);
}
