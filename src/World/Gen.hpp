#pragma once

#include "Core/Class.hpp"
#include "Core/Noise/Simplex.hpp"
#include "World/Chunk.hpp"

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

/**
 * Describe world generation.
 */
class GenDesc
{
    friend class Gen;
    
public:
    struct Buffer {
	String name;
	size_t element_size;
	bool flat;
    };
    
    void add_pass(Ref<GenPass> pass);

    void add_buffer(const String& name, size_t element_size, bool flat = false)
    {
	m_buffers.push_back(Buffer(name, element_size, flat));
    }

    View<Ref<GenPass>> passes() const
    {
	return View(m_passes.data(), m_passes.size());
    }
    
private:
    std::vector<Ref<GenPass>> m_passes;
    std::vector<Buffer> m_buffers;
};

/**
 * The world generator for a chunk.
 */
class Gen
{
public:
    Gen(GenDesc& desc)
	: m_desc(desc)
    {
	for (const GenDesc::Buffer& desc : desc.m_buffers) {
	    m_buffers.put(desc.name, alloc_array<uint8_t>(desc.element_size));
	}
    }

    ~Gen()
    {
	for (const auto& [_, name, buf] : m_buffers) {
	    destroy((uint8_t *)buf);
	}
    }

    const GenDesc& desc() const
    {
	return m_desc;
    }

    template<typename T>
    T *get_buffer(const StringView& name) const
    {
	return static_cast<T *>(m_buffers.get(name).value());
    }

private:
    GenDesc m_desc;
    HashMap<String, void *> m_buffers;
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
    virtual void gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags) = 0;

    bool is_flat() const
    {
	return m_flat;
    }
    
protected:
    /**
     * This pass should only be dispatch in 2D, not for each.
     */
    bool m_flat = false;
    SimplexNoise m_noise = 0;
};

class OverworldOceanPass : public GenPass
{
    CLASS(OverworldOceanPass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags) override;
};

class MountainPass : public GenPass
{
    CLASS(MountainPass, GenPass);

public:
    virtual void init(GenDesc& desc) override;
    virtual void gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags) override;
};

class OverworldTerrainPass : public GenPass
{
    CLASS(OverworldTerrainPass, GenPass);

public:
    virtual void gen(const Gen& gen, int64_t x, int64_t y, int64_t z, BlockState& state, BlockTags& tags) override;
};
