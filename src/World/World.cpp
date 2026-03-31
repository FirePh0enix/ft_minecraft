#include "World/World.hpp"
#include "AABB.hpp"
#include "Core/Class.hpp"
#include "Engine.hpp"
#include "Profiler.hpp"
#include "Render/Types.hpp"
#include "World/Chunk.hpp"
#include "World/Generator.hpp"
#include "World/Pass/Overworld.hpp"
#include "webgpu/webgpu.h"

World::World(uint64_t seed)
    : m_seed(seed)
{
    // m_env_buffer = RenderingDriver::get()->create_buffer(sizeof(Environment), BufferUsageFlagBits::Uniform | BufferUsageFlagBits::CopyDest).value_or(nullptr);

    // m_shader = Shader::load("assets/shaders/voxel.wgsl").value_or(nullptr);
    // m_shader->set_binding("images", Binding(BindingKind::Texture, ShaderStageFlagBits::Fragment, 0, 0, BindingAccess::Read, TextureDimension::D2DArray)); // binding = 1 is the sampler
    // m_shader->set_binding("env", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 2, BindingAccess::Read));
    // m_shader->set_binding("model", Binding(BindingKind::UniformBuffer, ShaderStageFlagBits::Vertex, 0, 3, BindingAccess::Read));

    // m_shader->set_sampler("images", {.min_filter = Filter::Nearest, .mag_filter = Filter::Nearest});

    // Setup world generation
    m_generators[overworld] = newobj(Generator, this, overworld);
    m_generators[overworld]->set_distance(8);

    m_generators[overworld]->add_pass(newobj(OverworldBiomePass));
    m_generators[overworld]->add_pass(newobj(OverworldSurfacePass));
}

World::~World()
{
}

void World::tick(float delta)
{
    ZoneScoped;

    // m_dims[0].get_physics_system().step();

    for (Ref<Entity> entity : m_dims[overworld].get_entities())
        entity->recurse_tick(delta);

    const glm::vec3 player_pos = m_camera->get_global_transform().position();
    m_generators[overworld]->set_reference_pos(player_pos);
    m_generators[overworld]->load_around(int64_t(player_pos.x), int64_t(player_pos.y), int64_t(player_pos.z));
}

void World::draw(bool include_entities)
{
    ZoneScoped;

    if (include_entities)
    {
        for (Ref<Entity> entity : m_dims[0].get_entities())
            entity->draw();
    }

    // std::lock_guard<std::mutex> guard(m_dims[0].mutex());

    // m_env.view_matrix = m_camera->get_view_proj_matrix();
    // update_environment_buffer();

    // for (const Ref<Chunk>& chunk : m_dims[0].get_visible_chunks())
    // {
    //     ZoneScopedN("draw.iterate");

    //     ChunkPos pos = chunk->pos();
    //     AABB aabb = AABB(glm::vec3((float)pos.x * Chunk::width + Chunk::width / 2.0, (float)pos.y * Chunk::width + Chunk::width / 2.0, (float)pos.z * Chunk::width + Chunk::width / 2.0),
    //                      glm::vec3(Chunk::width / 2.0, Chunk::width / 2, Chunk::width / 2));
    //     (void)aabb;

    //     // if (!m_camera->frustum().contains(aabb))
    //     //    continue;;

    //     encoder.bind_material(chunk->get_material());

    //     const Ref<Mesh>& mesh = chunk->get_mesh();

    //     encoder.bind_index_buffer(mesh->get_buffer(MeshBufferKind::Index));
    //     encoder.bind_vertex_buffer(mesh->get_buffer(MeshBufferKind::Position), 0);
    //     encoder.bind_vertex_buffer(mesh->get_buffer(MeshBufferKind::Normal), 1);
    //     encoder.bind_vertex_buffer(mesh->get_buffer(MeshBufferKind::UV), 2);

    //     encoder.draw(mesh->vertex_count(), 1);
    // }
}

BlockState World::get_block_state(int64_t x, int64_t y, int64_t z) const
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_y = y >= 0 ? (y / 16) : (y / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_y, chunk_z);

    if (!chunk_value.has_value())
    {
        return BlockState();
    }

    Ref<Chunk> chunk = chunk_value.value();
    const int64_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const int64_t chunk_local_y = y >= 0 ? (y % 16) : 16 + (y % 16);
    const int64_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    return chunk->get_block(chunk_local_x, chunk_local_y, chunk_local_z);
}

void World::set_block_state(int64_t x, int64_t y, int64_t z, BlockState state)
{
    int64_t chunk_x = x >= 0 ? (x / 16) : (x / 16 - 1);
    int64_t chunk_y = y >= 0 ? (y / 16) : (y / 16 - 1);
    int64_t chunk_z = z >= 0 ? (z / 16) : (z / 16 - 1);

    std::optional<Ref<Chunk>> chunk_value = get_chunk(chunk_x, chunk_y, chunk_z);

    if (!chunk_value.has_value())
    {
        return;
    }

    Ref<Chunk> chunk = chunk_value.value();
    const int64_t chunk_local_x = x >= 0 ? (x % 16) : 16 + (x % 16);
    const int64_t chunk_local_y = y >= 0 ? (y % 16) : 16 + (y % 16);
    const int64_t chunk_local_z = z >= 0 ? (z % 16) : 16 + (z % 16);

    chunk->set_block(chunk_local_x, chunk_local_y, chunk_local_z, state);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t y, int64_t z) const
{
    return m_dims[overworld].get_chunk(x, y, z);
}

std::optional<Ref<Chunk>> World::get_chunk(int64_t x, int64_t y, int64_t z)
{
    return m_dims[overworld].get_chunk(x, y, z);
}

void World::set_active_camera(Ref<Camera> camera)
{
    m_camera = camera;
    // update_environment_buffer();
}

inline void adjust_on_boundary(double rcomp, int64_t& vcomp, double dcomp, double eps = 1e-12)
{
    if (std::abs(rcomp - static_cast<double>(vcomp)) <= eps && dcomp < 0.0)
    {
        --vcomp;
    }
}

std::optional<RaycastResult> World::raycast(const Ray& ray, float range)
{
    const glm::vec3 ro = ray.origin();
    const glm::vec3 rd = ray.dir();

    // avoid zero direction
    if (ro == glm::vec3())
        return std::nullopt;

    // current voxel coordinates
    int64_t vx = (int)std::floor(ro.x);
    int64_t vy = (int)std::floor(ro.y);
    int64_t vz = (int)std::floor(ro.z);

    // if starting exactly on integer boundary and ray goes negative, move to neighbor voxel
    adjust_on_boundary(ro.x, vx, rd.x);
    adjust_on_boundary(ro.y, vy, rd.y);
    adjust_on_boundary(ro.z, vz, rd.z);

    // If starting inside a solid voxel, report immediate hit at t=0
    if (!get_block_state(vx, vy, vz).is_air())
    {
        // Determine closest face? common choice: return entry face as opposite of ray direction
        Face face;
        if (std::abs(rd.x) >= std::abs(rd.y) && std::abs(rd.x) >= std::abs(rd.z))
        {
            face = (rd.x > 0.0) ? Face::NegX : Face::PosX;
        }
        else if (std::abs(rd.y) >= std::abs(rd.x) && std::abs(rd.y) >= std::abs(rd.z))
        {
            face = (rd.y > 0.0) ? Face::NegY : Face::PosY;
        }
        else
        {
            face = (rd.z > 0.0) ? Face::NegZ : Face::PosZ;
        }
        return RaycastResult{vx, vy, vz, face, ro, 0.0};
    }

    // step direction: +1 or -1 per axis
    int64_t step_x = (rd.x > 0.0) ? 1 : (rd.x < 0.0) ? -1
                                                     : 0;
    int64_t step_y = (rd.y > 0.0) ? 1 : (rd.y < 0.0) ? -1
                                                     : 0;
    int64_t step_z = (rd.z > 0.0) ? 1 : (rd.z < 0.0) ? -1
                                                     : 0;

    // tMax: distance along ray to the first voxel boundary on each axis
    float tmax_x, tmax_y, tmax_z;
    // tDelta: distance along ray between subsequent crossings of voxel boundaries
    float tdelta_x, tdelta_y, tdelta_z;

    auto safe_div = [](float a, float b) -> float
    { return (b == 0.0) ? INFINITY : a / b; };

    // compute initial tMax for X
    if (step_x != 0)
    {
        float nextBoundaryX = float(vx + (step_x > 0 ? 1 : 0));
        tmax_x = safe_div(nextBoundaryX - ro.x, rd.x);
        tdelta_x = std::abs(safe_div(1.0, rd.x));
    }
    else
    {
        tmax_x = INFINITY;
        tdelta_x = INFINITY;
    }

    if (step_y != 0)
    {
        float nextBoundaryY = float(vy + (step_y > 0 ? 1 : 0));
        tmax_y = safe_div(nextBoundaryY - ro.y, rd.y);
        tdelta_y = std::abs(safe_div(1.0, rd.y));
    }
    else
    {
        tmax_y = INFINITY;
        tdelta_y = INFINITY;
    }

    if (step_z != 0)
    {
        float nextBoundaryZ = float(vz + (step_z > 0 ? 1 : 0));
        tmax_z = safe_div(nextBoundaryZ - ro.z, rd.z);
        tdelta_z = std::abs(safe_div(1.0, rd.z));
    }
    else
    {
        tmax_z = INFINITY;
        tdelta_z = INFINITY;
    }

    float t = 0.0;
    const int max_steps = 100000; // safety cap
    for (int i = 0; i < max_steps && t <= range; ++i)
    {
        // advance to next voxel boundary
        if (tmax_x < tmax_y)
        {
            if (tmax_x < tmax_z)
            {
                // step X
                vx += step_x;
                t = tmax_x;
                tmax_x += tdelta_x;
                // entering through ±X face
                Face face = (step_x > 0) ? Face::NegX : Face::PosX;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::dvec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
            else
            {
                // step Z
                vz += step_z;
                t = tmax_z;
                tmax_z += tdelta_z;
                Face face = (step_z > 0) ? Face::NegZ : Face::PosZ;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::dvec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
        }
        else
        {
            if (tmax_y < tmax_z)
            {
                // step Y
                vy += step_y;
                t = tmax_y;
                tmax_y += tdelta_y;
                Face face = (step_y > 0) ? Face::NegY : Face::PosY;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::vec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
            else
            {
                // step Z
                vz += step_z;
                t = tmax_z;
                tmax_z += tdelta_z;
                Face face = (step_z > 0) ? Face::NegZ : Face::PosZ;
                if (t > range)
                    break;
                if (!get_block_state(vx, vy, vz).is_air())
                {
                    glm::vec3 pos = ro + rd * t;
                    return RaycastResult{vx, vy, vz, face, pos, t};
                }
            }
        }
    }

    return std::nullopt;
}

void World::update_internal_buffers()
{
    if (m_node_buffer != nullptr || m_dims[0].get_visible_chunks().size() == 0)
    {
        return;
    }

    // const Ref<Chunk>& chunk = m_dims[0].get_visible_chunks()[0];

    // Lets create a tree from scratch !z
    std::vector<SvtNode64> nodes;    // = chunk->m_nodes;
    std::vector<uint32_t> leaf_data; // = chunk->m_leafs;

    leaf_data.push_back(0); // red

    // master => child_l0 => child_l1

    SvtNode64 node{};
    node.leaf = 0; // master
    node.child_ptr = 1;
    node.child_mask = 0b1;
    nodes.push_back(node);

    node.leaf = 0; // l0
    node.child_ptr = 2;
    node.child_mask = 0b1;
    nodes.push_back(node);

    node.leaf = 1;
    node.child_ptr = 0;
    node.child_mask = 0;
    nodes.push_back(node);

    WGPUBufferDescriptor desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Storage,
        .size = 0,
        .mappedAtCreation = false,
    };

    desc.size = sizeof(SvtNode64) * nodes.size();
    m_node_buffer = wgpuDeviceCreateBuffer(Engine::get().get_renderer().m_device, &desc);
    wgpuQueueWriteBuffer(Engine::get().get_renderer().m_queue, m_node_buffer, 0, nodes.data(), sizeof(SvtNode64) * nodes.size());

    desc.size = sizeof(uint32_t) * leaf_data.size();
    m_leaf_buffer = wgpuDeviceCreateBuffer(Engine::get().get_renderer().m_device, &desc);
    wgpuQueueWriteBuffer(Engine::get().get_renderer().m_queue, m_leaf_buffer, 0, leaf_data.data(), sizeof(uint32_t) * leaf_data.size());

    const WGPUBindGroupEntry entries[]{
        WGPUBindGroupEntry{.binding = 0, .buffer = m_node_buffer, .offset = 0, .size = sizeof(SvtNode64) * nodes.size()},
        WGPUBindGroupEntry{.binding = 1, .buffer = m_leaf_buffer, .offset = 0, .size = sizeof(uint32_t) * leaf_data.size()},
    };
    WGPUBindGroupDescriptor bg_desc{
        .nextInChain = nullptr,
        .label = WGPU_STRING_VIEW_INIT,
        .layout = Engine::get().get_renderer().m_rv_raytracing_node_bind_group_layout,
        .entryCount = sizeof(entries) / sizeof(WGPUBindGroupEntry),
        .entries = entries,
    };
    m_bind_group = wgpuDeviceCreateBindGroup(Engine::get().get_renderer().m_device, &bg_desc);
}

// void World::update_environment_buffer()
// {
//     m_env_buffer->update(View(m_env).as_bytes());
// }
