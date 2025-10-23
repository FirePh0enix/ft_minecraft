/*
    
 */

struct Block
{
    uint16_t id;
    uint16_t raw;
};

struct GeneralInfo
{
    uint chunk_x;
    uint chunk_y;
};

ConstantBuffer<GeneralInfo> info;
RWStructuredBuffer<Block> blocks;

[shader("compute")]
[numthreads(16, 256, 16)]
void computeMain(uint3 id : SV_GroupThreadID)
{
    uint index = id.z * 16 * 256 + id.y * 16 + id.x;

    if (id.y == 0)
        blocks[index] = Block(1, 0);
}
