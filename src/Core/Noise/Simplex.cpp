#include "Core/Noise/Simplex.hpp"

#include <cstddef>
#include <random>

SimplexNoise::SimplexNoise(uint64_t seed)
    : m_perms()
{
    for (size_t i = 0; i < 256; i++)
        m_perms[i] = i;

    std::mt19937 prng{std::random_device{}()};
    prng.seed(seed);
}

float SimplexNoise::sample(glm::vec2 coords) const
{
    float x = coords.x;
    float y = coords.y;

    float n0, n1, n2;

    const float F2 = 0.366025403f;
    const float G2 = 0.211324865f;

    float s = (x + y) * F2;
    float xs = x + s;
    float ys = y + s;
    int i = glm::floor(xs);
    int j = glm::floor(ys);

    float t = float(i + j) * G2;
    float X0 = float(i) - t;
    float Y0 = float(j) - t;
    float x0 = x - X0;
    float y0 = y - Y0;

    int i1, j1;
    if (x0 > y0)
    {
        i1 = 1;
        j1 = 0;
    }
    else
    {
        i1 = 0;
        j1 = 1;
    }

    float x1 = x0 - float(i1) + G2;
    float y1 = y0 - float(j1) + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    int gi0 = m_perms[i + m_perms[j]];
    int gi1 = m_perms[i + i1 + m_perms[j + j1]];
    int gi2 = m_perms[i + 1 + m_perms[j + 1]];

    float t0 = 0.5f - x0 * x0 - y0 * y0;
    if (t0 < 0.0)
        n0 = 0.0;
    else
    {
        t0 *= t0;
        n0 = t0 * t0 * gradient(gi0, x0, y0);
    }

    float t1 = 0.5f - x1 * x1 - y1 * y1;
    if (t1 < 0.0)
        n1 = 0.0;
    else
    {
        t1 *= t1;
        n1 = t1 * t1 * gradient(gi1, x1, y1);
    }
    float t2 = 0.5f - x2 * x2 - y2 * y2;
    if (t2 < 0.0)
        n2 = 0.0;
    else
    {
        t2 *= t2;
        n2 = t2 * t2 * gradient(gi2, x2, y2);
    }
    return 45.23065f * (n0 + n1 + n2);
}

float SimplexNoise::sample(glm::vec3 coords) const
{
    float x = coords.x;
    float y = coords.y;
    float z = coords.z;

    float n0, n1, n2, n3;

    const float F3 = 1.0 / 3.0;
    const float G3 = 1.0 / 6.0;

    float s = (x + y + z) * F3;
    int i = glm::floor(x + s);
    int j = glm::floor(y + s);
    int k = glm::floor(z + s);

    float t = float(i + j + k) * G3;
    float X0 = float(i) - t;
    float Y0 = float(j) - t;
    float Z0 = float(k) - t;
    float x0 = x - X0;
    float y0 = y - Y0;
    float z0 = z - Z0;

    int i1, j1, k1;
    int i2, j2, k2;

    if (x0 >= y0)
    {
        if (y0 >= z0)
        {
            i1 = 1;
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;
        }
        else if (x0 >= z0)
        {
            i1 = 1;
            j1 = 0;
            k1 = 0;
            i2 = 1;
            j2 = 0;
            k2 = 1;
        }
        else
        {
            i1 = 0;
            j1 = 0;
            k1 = 1;
            i2 = 1;
            j2 = 0;
            k2 = 1;
        }
    }
    else
    {
        if (y0 < z0)
        {
            i1 = 0;
            j1 = 0;
            k1 = 1;
            i2 = 0;
            j2 = 1;
            k2 = 1;
        }
        else if (x0 < z0)
        {
            i1 = 0;
            j1 = 1;
            k1 = 0;
            i2 = 0;
            j2 = 1;
            k2 = 1;
        }
        else
        {
            i1 = 0;
            j1 = 1;
            k1 = 0;
            i2 = 1;
            j2 = 1;
            k2 = 0;
        }
    }

    float x1 = x0 - float(i1) + G3;
    float y1 = y0 - float(j1) + G3;
    float z1 = z0 - float(k1) + G3;
    float x2 = x0 - float(i2) + 2.0f * G3;
    float y2 = y0 - float(j2) + 2.0f * G3;
    float z2 = z0 - float(k2) + 2.0f * G3;
    float x3 = x0 - 1.0f + 3.0f * G3;
    float y3 = y0 - 1.0f + 3.0f * G3;
    float z3 = z0 - 1.0f + 3.0f * G3;

    int gi0 = int(m_perms[i + m_perms[j + m_perms[k]]]);
    int gi1 = int(m_perms[i + i1 + m_perms[j + j1 + m_perms[k + k1]]]);
    int gi2 = int(m_perms[i + i2 + m_perms[j + j2 + m_perms[k + k2]]]);
    int gi3 = int(m_perms[i + 1 + m_perms[j + 1 + m_perms[k + 1]]]);

    float t0 = 0.6f - glm::dot(glm::vec3(x0, y0, z0), glm::vec3(x0, y0, z0));
    n0 = (t0 < 0.0) ? 0.0f : glm::pow(t0, 4.0f) * gradient(gi0, x0, y0, z0);
    float t1 = 0.6f - glm::dot(glm::vec3(x1, y1, z1), glm::vec3(x1, y1, z1));
    n1 = (t1 < 0.0) ? 0.0f : glm::pow(t1, 4.0f) * gradient(gi1, x1, y1, z1);
    float t2 = 0.6f - glm::dot(glm::vec3(x2, y2, z2), glm::vec3(x2, y2, z2));
    n2 = (t2 < 0.0) ? 0.0f : glm::pow(t2, 4.0f) * gradient(gi2, x2, y2, z2);
    float t3 = 0.6f - glm::dot(glm::vec3(x3, y3, z3), glm::vec3(x3, y3, z3));
    n3 = (t3 < 0.0) ? 0.0f : glm::pow(t3, 4.0f) * gradient(gi3, x3, y3, z3);

    return 32.0f * (n0 + n1 + n2 + n3);
}
