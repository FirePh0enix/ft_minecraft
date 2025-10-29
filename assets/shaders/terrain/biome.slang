/*
    Generate biomes.
 */

enum Biome
{
    River,
    Beach,
    Desert,
    Plains,
    Snow,
    Ocean
};

RWStructuredBuffer<Biome> biomes;

[shader("compute")]
[numthreads(16, 1, 16)]
void computeMain(uint3 id: SV_GroupThreadID)
{
    uint index = id.z * 16 * 256 + id.y * 16 + id.x;
}
