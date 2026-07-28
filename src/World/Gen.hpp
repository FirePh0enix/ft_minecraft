#pragma once

#include "Core/Class.hpp"
#include "Core/Noise/Simplex.hpp"
#include "World/Chunk.hpp"
#include "World/Settings.hpp"
#include "World/Structure.hpp"

#include "spline.hpp"

//
// Separate multiple type of passes:
// - PreGenPass   => Generate biome, elevation, and other buffer values.
// - RealizePass  => From previous passes, place blocs
//     - TerrainPass => Place terrain blocs.
//     - CavePass    => Carve caves in the ground.
//     - WaterPass   => Place water blocs to create oceans and more.
// - FeaturePass         => Add features like trees, tall grass and more.
// - CompositeStructPass => Add structures that needs to be compositd, like villages or dungeons.
//

class GenPass;
class StructurePass;

// defined in spline.c from https://oceancolor.gsfc.nasa.gov/staff/norman/seawifs_image_cookbook/faux_shuttle/spline.c
extern void spline(
    float x[],
    float y[],
    int n,
    float yp1,
    float ypn,
    float y2[]);
extern void splint(
    float xa[],
    float ya[],
    float y2a[],
    int n,
    float x,
    float *y);

/**
 * Describe world generation.
 */
class GenDesc
{
    friend class Gen;

public:
    struct Buffer
    {
        std::string name;
        size_t element_size;
        bool flat;
    };

    GenDesc(WorldSettings settings)
        : m_settings(settings)
    {
    }

    GenDesc(const GenDesc&) = delete;

    void add_pass(std::shared_ptr<GenPass> pass);
    void add_struct_pass(std::shared_ptr<StructurePass> pass);

    void add_buffer(const std::string& name, size_t element_size, bool flat = false)
    {
        m_buffers.push_back(Buffer(name, element_size, flat));
    }

    std::span<std::shared_ptr<GenPass>> passes()
    {
        return std::span<std::shared_ptr<GenPass>>(m_passes);
    }

    std::span<std::shared_ptr<StructurePass>> spasses()
    {
        return std::span<std::shared_ptr<StructurePass>>(m_struct_passes);
    }

    WorldSettings settings() const { return m_settings; }

private:
    std::vector<std::shared_ptr<GenPass>> m_passes;
    std::vector<std::shared_ptr<StructurePass>> m_struct_passes;
    std::vector<Buffer> m_buffers;
    WorldSettings m_settings;
};

/**
 * The world generator for a chunk.
 */
class Gen
{
public:
    Gen(std::shared_ptr<GenDesc> desc)
        : m_desc(desc)
    {
        for (const GenDesc::Buffer& desc : desc->m_buffers)
            m_buffers[desc.name] = new uint8_t[desc.element_size];
    }

    ~Gen()
    {
        for (const auto& [name, buf] : m_buffers)
            delete[] (uint8_t *)buf;
    }

    std::shared_ptr<GenDesc> desc()
    {
        return m_desc;
    }

    WorldSettings settings()
    {
        return desc()->settings();
    }

    template <typename T>
    T *get_buffer(std::string_view name) const
    {
        return static_cast<T *>(m_buffers.find(name)->second);
    }

private:
    std::shared_ptr<GenDesc> m_desc;
    stdext::string_map<void *> m_buffers;
};

/**
 * Base of all generation pass.
 */
class GenPass : public Object
{
    CLASS(GenPass, Object);

public:
    virtual void init(GenDesc& desc)
    {
        (void)desc;
    }
    virtual void gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome) = 0;

    bool is_flat() const
    {
        return m_flat;
    }

protected:
    /**
     * This pass should only be dispatch in 2D, not for each blocks.
     */
    bool m_flat = false;
    SimplexNoise m_noise = 0;
};

class OverworldOceanPass : public GenPass
{
    CLASS(OverworldOceanPass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome) override;
};

class MountainPass : public GenPass
{
    CLASS(MountainPass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome) override;
};

class OverworldBiomePass : public GenPass
{
    CLASS(BiomePass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome) override;
};

class HeightPass : public GenPass
{
    CLASS(HeightPass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome) override;

private:
    tk::spline m_spline;
    tk::spline m_mspline;
    tk::spline m_rspline;
};

class OverworldTerrainPass : public GenPass
{
    CLASS(OverworldTerrainPass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags, Biome& biome) override;
};

class StructurePass : public Object
{
    CLASS(StructurePass, Object);

public:
    virtual void init(GenDesc& desc)
    {
        (void)desc;
    }
    virtual void gen(const Gen& gen, std::shared_ptr<Chunk> chunk) = 0;
};

class TreePass : public StructurePass
{
    CLASS(TreePass, StructurePass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(const Gen& gen, std::shared_ptr<Chunk> chunk) override;

private:
    std::shared_ptr<Structure> m_tree_struct;
};
