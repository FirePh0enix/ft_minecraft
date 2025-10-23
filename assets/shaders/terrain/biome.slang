/*
    Generate biomes.
 */

enum Biome
{
    FrozenRiver,
    River,
    StonyShore,
    StonyPeaks,
    JaggedPeaks,
    FrozenPeaks,
    // (Beach Biomes)
    Beach,
    Desert,
    // (Middle Biomes)
    SnowyPlains,
    Taiga,
    Plains,
    Savanna,
    // (Non inland Biomes)
    FrozenOcean,
    Ocean
};

RWStructuredBuffer<Biome> biomes;

[shader("compute")]
[numthreads(16, 1, 16)]
void computeMain(uint3 id: SV_GroupThreadID)
{
    uint index = id.z * 16 * 256 + id.y * 16 + id.x;
}
