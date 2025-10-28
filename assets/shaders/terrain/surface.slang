// This structure need to be kept in sync with CPU code.
struct BlockState
{
    uint id_raw;
};

// This structure need to be kept in sync with CPU code.
struct GeneralInfo
{
    uint32_t chunk_x;
    uint32_t chunk_z;
};

ConstantBuffer<GeneralInfo> info;
RWStructuredBuffer<BlockState> blocks;

[shader("compute")]
[numthreads(16, 1, 16)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    uint index = id.z * 16 * 256 + id.y * 16 + id.x;
    uint3 globalPosition = uint3(id.x + info.chunk_x * 16, id.y, id.z + info.chunk_z);

    BlockState block;

    if (id.y == 0)
        block = BlockState(1);
    else
        block = BlockState(0);

    blocks[index] = block;
}
