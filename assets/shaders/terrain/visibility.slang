import defs;

#define NORTH_MASK  0b1
#define SOUTH_MASK  0b10
#define WEST_MASK   0b1000
#define EAST_MASK   0b100
#define TOP_MASK    0b10000
#define BOTTOM_MASK 0b100000

StructuredBuffer<BlockState> blocks;
RWStructuredBuffer<uint> visibilityBuffer;

inline uint index(uint x, uint y, uint z)
{
    return z * 16 * 256 + y * 16 + x;
}

// void setVisibility(uint idx, uint val)
// {
//     uint e = idx >> 4;
//     uint w = (idx >> 2) & 3;
//     uint b = idx & 3;
//     uint shift = b * 8;
//     uint mask = 0xFFu << shift;
//     visibilityBuffer[e][w] = (visibilityBuffer[e][w] & ~mask) | ((val & 0xFFu) << shift);
// }

[shader("compute")]
[numthreads(1, 256, 1)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint visibility = 0x0;

    if (blocks[index(id.x, id.y, id.z)].isAir())
        return;

    if (id.y == 0 || blocks[index(id.x, id.y - 1, id.z)].isAir())
        visibility |= BOTTOM_MASK;
    if (id.y == 255 || blocks[index(id.x, id.y + 1, id.z)].isAir())
        visibility |= TOP_MASK;

    if (id.x == 0 || blocks[index(id.x - 1, id.y, id.z)].isAir())
        visibility |= EAST_MASK;
    if (id.x == 15 || blocks[index(id.x + 1, id.y, id.z)].isAir())
        visibility |= WEST_MASK;

    if (id.z == 0 || blocks[index(id.x, id.y, id.z - 1)].isAir())
        visibility |= NORTH_MASK;
    if (id.z == 15 || blocks[index(id.x, id.y, id.z + 1)].isAir())
        visibility |= SOUTH_MASK;

    visibilityBuffer[index(id.x, id.y, id.z)] = visibility;
    // setVisibility(index(id.x, id.y, id.z), visibility);
}
