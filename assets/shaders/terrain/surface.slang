import defs;
import noise;

// This structure need to be kept in sync with CPU code.
struct GeneralInfo
{
    int32_t chunkX;
    int32_t chunkZ;
};

RWStructuredBuffer<BlockState> blocks;
ConstantBuffer<SimplexNoiseState> simplexState;

[push_constant]
GeneralInfo info;

static float evaluateSpline(const float keys[5], const float vals[5], int count, float x)
{
    if (x <= keys[0]) 
    {
        return vals[0];
    }
    if (x >= keys[count - 1]) 
    {
        return vals[count - 1];
    }

    for (int i = 0; i < count - 1; i++)
    {
        if (x >= keys[i] && x <= keys[i + 1])
        {
            float t = (x - keys[i]) / (keys[i + 1] - keys[i]);
            return lerp(vals[i], vals[i+1], t);
        }
    }
    return vals[count - 1];
}


static const int SPLINE_COUNT = 5;
static const float SEA_LEVEL = 62.0f;

static const float CONT_KEYS[5] = { -1.0f, -0.2f, -0.05f, 0.2f, 1.0f };
static const float CONT_VALS[5] = { 20.0f, 52.0f, 84.0f, 116.0f, 148.0f };

static const float ERO_KEYS[5]  = { -1.0f, -0.2f, 0.0f, 0.5f, 1.0f };
static const float ERO_VALS[5]  = { 1.8f, 1.2f, 1.0f, 0.75f, 0.5f };

static const float PV_KEYS[5]   = { -1.0f, -0.2f, 0.0f, 0.2f, 1.0f };
static const float PV_VALS[5]   = { -50.0f, -25.0f, 0.0f, 25.0f, 50.0f };


static float getContinentalness(int x, int z)
{
    return simplexState.fractal<2>(float3(x * 0.00025f, 0, z * 0.00025f), 1, 1, 2, 0.5); // simplexState.simplex2D(float2(x * 0.00052f, z * 0.00052f));
}

static float getErosion(int x, int z)
{
    return simplexState.fractal<2>(float3(x * 0.001f, 0, z * 0.001f), 1, 1, 2, 0.5); // simplexState.simplex2D(float2(x * 0.001f, z * 0.001f));
}


static float getPV(int x, int z)
{
    float w = simplexState.fractal<2>(float3(x * 0.0025f, 0, z * 0.0025f), 1, 1, 2, 0.5); // simplexState.simplex2D(float2(x * 0.0025f, z * 0.0025f));
    float a = abs(w);
    float pv = 1.0 - abs(3.0 * a - 2.0);
    return clamp(pv, -1.0, 1.0);
}

static float getOverhang(int x, int y, int z)
{
    float n = simplexState.simplex3D(float3(x * 0.03f, y * 0.03f, z * 0.03f));
    n += simplexState.simplex3D(float3(x * 0.06f, y * 0.06f, z * 0.06f)) * 0.5f;
    return n;
}

static float getCaveNoise(int x, int y, int z)
{
    float n = simplexState.simplex3D(float3(x * 0.003f, y * 0.003f, z * 0.003f)) * 2.0f;
    n += simplexState.simplex3D(float3(x * 0.006f, y * 0.006f, z * 0.006f)) * 1.0f;
    return n;
}

static const uint STONE = 3;
static const uint WATER = 5;

[shader("compute")]
[numthreads(16, 1, 16)]
void computeMain(uint3 id : SV_DispatchThreadID)
{
    int3 globalPosition = int3(id.x + info.chunkX * 16, id.y, id.z + info.chunkZ * 16);

    float continentalness = getContinentalness(globalPosition.x, globalPosition.z);
    float erosion = getErosion(globalPosition.x, globalPosition.z);
    float pv = getPV(globalPosition.x, globalPosition.z);

    float baseHeight = evaluateSpline(CONT_KEYS, CONT_VALS, SPLINE_COUNT, continentalness);
    float erosionFactor = evaluateSpline(ERO_KEYS, ERO_VALS, SPLINE_COUNT, erosion);
    float pvOffset = evaluateSpline(PV_KEYS, PV_VALS, SPLINE_COUNT, pv);

    float pv_scale = 1.0f;
    float height = baseHeight + pvOffset * erosionFactor * pv_scale;

    for (uint y = 0; y < 256; y++)
    {
        uint index = id.z * 16 * 256 + y * 16 + id.x;

        float overhang = getOverhang(globalPosition.x, y, globalPosition.z) * 15.0f - 10.0f;
        float density = height - y + overhang;

        float depthBelowSurface = height - y;
        float caveNoise = getCaveNoise(globalPosition.x, y, globalPosition.z);
        bool cave = (caveNoise > 0.7f) && (depthBelowSurface > 10.0f);

        if (cave)
        {
            // blocks[index].id_raw = 6;
            continue;
        }
        else if (density > 0.0f)
        {
            blocks[index].id_raw = STONE;
        }
        else if (y <= SEA_LEVEL)
        {
            blocks[index].id_raw = WATER;
        }
    }
}
