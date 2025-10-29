public struct SimplexNoiseState
{
    uint4 perms[16];

    inline uint32_t get_perm(uint index)
    {
        return (perms[index / 16][index % 16] >> (((index % 16) % 4) * 8)) & 0xFF;
    }

    public float simplex2D(float2 coords)
    {
        return 0.0;
    }
};
