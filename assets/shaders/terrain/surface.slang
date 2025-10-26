// This structure need to be kept in sync with CPU code.
struct Block
{
    uint id_raw;
};

// This structure need to be kept in sync with CPU code.
struct GeneralInfo
{
    uint32_t chunk_x;
    uint32_t chunk_z;
};

// This structure need to be kept in sync with CPU code.
struct BlockInstanceData
{
    float3 instance_position;
    uint3 textures;
    uint visibility_biome_gradient;
};

ConstantBuffer<GeneralInfo> info;
RWStructuredBuffer<Block> blocks;
RWStructuredBuffer<BlockInstanceData> instances;

[shader("compute")]
[numthreads(16, 1, 16)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint index = id.z * 16 * 256 + id.y * 16 + id.x;

    Block block;

    if (id.y == 0)
        block = Block(1);
    else
        block = Block(0);


    // TODO: Add a way to access which textures goes with which block in the GPU.

    uint visibility = block.id_raw == 0 ? 0 : 0xFFFFFFFF;
    
    blocks[index] = block;
    instances[index] = BlockInstanceData(float3(id.x + info.chunk_x * 16, id.y, id.z + info.chunk_z * 16), uint3(1, 1, 1), visibility);
}
