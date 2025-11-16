import defs;

StructuredBuffer<BlockState> blocks;
RWStructuredBuffer<uint4> visibilityBuffer;

inline uint index(uint x, uint y, uint z)
{
    return z * 16 * 256 + y * 16 + x;
}

inline void set_visibility(uint index, uint value)
{
    visibilityBuffer[index / 4] = (visibilityBuffer[index / 4] & ~(0xFF << ((index % 4) * 8))) | ((value & 0xFF) << ((index % 4) * 8));
}

[shader("compute")]
[numthreads(16, 256, 16)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint visibility = 0;

    if (id.y == 0 || blocks[index(id.x, id.y - 1, id.z)].id() != 0)
        visibility |= BOTTOM_MASK;
    if (id.y == 255 || blocks[index(id.x, id.y + 1, id.z)].id() != 0)
        visibility |= TOP_MASK;

    if (id.x == 0 || blocks[index(id.x - 1, id.y, id.z)].id() != 0)
        visibility |= EAST_MASK;
    if (id.x == 15 || blocks[index(id.x + 1, id.y, id.z)].id() != 0)
        visibility |= WEST_MASK;

    if (id.z == 0 || blocks[index(id.x, id.y, id.z - 1)].id() != 0)
        visibility |= NORTH_MASK;
    if (id.z == 15 || blocks[index(id.x, id.y, id.z + 1)].id() != 0)
        visibility |= SOUTH_MASK;

    set_visibility(index(id.x, id.y, id.z), visibility);
}
