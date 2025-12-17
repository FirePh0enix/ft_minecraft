#include "World/Pass/Surface.hpp"
#include "Profiler.hpp"
#include "World/Registry.hpp"

static const int SPLINE_COUNT = 5;
static const int64_t sea_level = 62;

static const float CONT_KEYS[5] = {-1.0f, -0.2f, -0.05f, 0.2f, 1.0f};
static const float CONT_VALS[5] = {20.0f, 52.0f, 84.0f, 116.0f, 148.0f};

static const float ERO_KEYS[5] = {-1.0f, -0.2f, 0.0f, 0.5f, 1.0f};
static const float ERO_VALS[5] = {1.8f, 1.2f, 1.0f, 0.75f, 0.5f};

static const float PV_KEYS[5] = {-1.0f, -0.2f, 0.0f, 0.2f, 1.0f};
static const float PV_VALS[5] = {-50.0f, -25.0f, 0.0f, 25.0f, 50.0f};

SurfacePass::SurfacePass()
{
    m_stone = BlockRegistry::get_block_id("stone");
    m_water = BlockRegistry::get_block_id("water");
}

void SurfacePass::seed_updated()
{
    m_simplex = SimplexNoise(m_seed);
}

static float evaluate_spline(const float keys[5], const float vals[5], int count, float x)
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
            return std::lerp(vals[i], vals[i + 1], t);
        }
    }
    return vals[count - 1];
}

static float get_continentalness(const SimplexNoise& simplex, int64_t x, int64_t z)
{
    return simplex.fractal<2>(glm::vec3(x * 0.00025f, 0, z * 0.00025f), 1, 1, 2, 0.5); // simplexState.simplex2D(float2(x * 0.00052f, z * 0.00052f));
}

static float get_erosion(const SimplexNoise& simplex, int64_t x, int64_t z)
{
    return simplex.fractal<2>(glm::vec3(x * 0.001f, 0, z * 0.001f), 1, 1, 2, 0.5); // simplexState.simplex2D(float2(x * 0.001f, z * 0.001f));
}

static float get_pv(const SimplexNoise& simplex, int64_t x, int64_t z)
{
    float w = simplex.fractal<2>(glm::vec3(x * 0.0025f, 0, z * 0.0025f), 1, 1, 2, 0.5); // simplexState.simplex2D(float2(x * 0.0025f, z * 0.0025f));
    float a = std::abs(w);
    float pv = 1.0f - std::abs(3.0f * a - 2.0f);
    return glm::clamp(pv, -1.0f, 1.0f);
}

static float get_overhang(const SimplexNoise& simplex, int64_t x, int64_t y, int64_t z)
{
    float n = simplex.sample(glm::vec3(x * 0.03f, y * 0.03f, z * 0.03f));
    n += simplex.sample(glm::vec3(x * 0.06f, y * 0.06f, z * 0.06f)) * 0.5f;
    return n;
}

static float get_cave_noise(const SimplexNoise& simplex, int64_t x, int64_t y, int64_t z)
{
    float n = simplex.sample(glm::vec3(x * 0.003f, y * 0.003f, z * 0.003f)) * 2.0f;
    n += simplex.sample(glm::vec3(x * 0.006f, y * 0.006f, z * 0.006f)) * 1.0f;
    return n;
}

void SurfacePass::process(int64_t x, int64_t z, int64_t cx, int64_t cz, std::vector<BlockState>& blocks) const
{
    ZoneScoped;

    float continentalness = get_continentalness(m_simplex, x, z);
    float erosion = get_erosion(m_simplex, x, z);
    float pv = get_pv(m_simplex, x, z);

    float base_height = evaluate_spline(CONT_KEYS, CONT_VALS, SPLINE_COUNT, continentalness);
    float erosion_factor = evaluate_spline(ERO_KEYS, ERO_VALS, SPLINE_COUNT, erosion);
    float pv_offset = evaluate_spline(PV_KEYS, PV_VALS, SPLINE_COUNT, pv);

    float pv_scale = 1.0f;
    float height = base_height + pv_offset * erosion_factor * pv_scale;

    for (int64_t y = 0; y < (int64_t)height; y++)
    {
        BlockState block;
        float fy = (float)y;

        float overhang = get_overhang(m_simplex, x, y, z) * 15.0f - 10.0f;
        float density = height - fy + overhang;

        float depth_below_surface = height - fy;
        float cave_noise = get_cave_noise(m_simplex, x, y, z);
        bool cave = (cave_noise > 0.7f) && (depth_below_surface > 10.0f);

        if (cave)
            block = BlockState();
        else if (density > 0.0f)
            block = BlockState(m_stone);
        else if (y <= sea_level)
            block = BlockState(m_water);

        blocks[cz * 16 * 256 + y * 16 + cx] = block;
    }
}
