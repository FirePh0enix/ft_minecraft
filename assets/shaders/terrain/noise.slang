public struct SimplexNoiseState
{
    uint4 perms[16];

    inline uint32_t get_perm(uint index)
    {
        return (perms[index >> 4][index >> 2] >> (((index & 0xF) & 3) * 8)) & 0xFF;
    }

    inline int fastFloor(float x)
    {
        int i = int(x);
        return (x < i) ? (i - 1) : i;
    }

    inline float grad1D(int hash, float x)
    {
        int h = hash & 0x0F;
        float grad = 1.0 + float(h & 7);
        if ((h & 8) != 0) grad = -grad;
        return grad * x;
    }

    inline float grad2D(int hash, float x, float y)
    {
        int h = hash & 0x3F;
        float u = (h < 4) ? x : y;
        float v = (h < 4) ? y : x;
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -2.0 * v : 2.0 * v);
    }

    inline float grad3D(int hash, float x, float y, float z)
    {
        int h = hash & 15;
        float u = (h < 8) ? x : y;
        float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
        return ((h & 1) != 0 ? -u : u) + ((h & 2) != 0 ? -v : v);
    }

    
    public float simplex2D(float2 coords)
    {
        float x = coords.x;
        float y = coords.y;

        float n0, n1, n2;

        const float F2 = 0.366025403f; 
        const float G2 = 0.211324865f; 

        float s = (x + y) * F2;
        float xs = x + s;
        float ys = y + s;
        int i = fastFloor(xs);
        int j = fastFloor(ys);

        float t = float(i + j) * G2;
        float X0 = float(i) - t;
        float Y0 = float(j) - t;
        float x0 = x - X0;
        float y0 = y - Y0;

        int i1, j1;
        if (x0 > y0) { i1 = 1; j1 = 0; }
        else         { i1 = 0; j1 = 1; }

        float x1 = x0 - float(i1) + G2;
        float y1 = y0 - float(j1) + G2;
        float x2 = x0 - 1.0 + 2.0 * G2;
        float y2 = y0 - 1.0 + 2.0 * G2;

        int gi0 = int(get_perm(i + get_perm(j)));
        int gi1 = int(get_perm(i + i1 + get_perm(j + j1)));
        int gi2 = int(get_perm(i + 1 + get_perm(j + 1)));

        float t0 = 0.5 - x0*x0 - y0*y0;
        if (t0 < 0.0)
            n0 = 0.0;
        else {
            t0 *= t0;
            n0 = t0 * t0 * grad2D(gi0, x0, y0);
        }

        float t1 = 0.5 - x1*x1 - y1*y1;
        if (t1 < 0.0)
            n1 = 0.0;
        else {
            t1 *= t1;
            n1 = t1 * t1 * grad2D(gi1, x1, y1);
        }
        float t2 = 0.5 - x2*x2 - y2*y2;
        if (t2 < 0.0)
            n2 = 0.0;
        else {
            t2 *= t2;
            n2 = t2 * t2 * grad2D(gi2, x2, y2);
        }
        return 45.23065f * (n0 + n1 + n2);
    }

    public float simplex3D(float3 coords)
    {
        float x = coords.x;
        float y = coords.y;
        float z = coords.z;

        float n0, n1, n2, n3;

        const float F3 = 1.0 / 3.0;
        const float G3 = 1.0 / 6.0;

        float s = (x + y + z) * F3;
        int i = fastFloor(x + s);
        int j = fastFloor(y + s);
        int k = fastFloor(z + s);

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
            { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 1; k2 = 0; }
            else if (x0 >= z0)
            { i1 = 1; j1 = 0; k1 = 0; i2 = 1; j2 = 0; k2 = 1; }
            else
            { i1 = 0; j1 = 0; k1 = 1; i2 = 1; j2 = 0; k2 = 1; }
        }
        else
        {
            if (y0 < z0)
            { i1 = 0; j1 = 0; k1 = 1; i2 = 0; j2 = 1; k2 = 1; }
            else if (x0 < z0)
            { i1 = 0; j1 = 1; k1 = 0; i2 = 0; j2 = 1; k2 = 1; }
            else
            { i1 = 0; j1 = 1; k1 = 0; i2 = 1; j2 = 1; k2 = 0; } 
        }

        float x1 = x0 - float(i1) + G3;
        float y1 = y0 - float(j1) + G3;
        float z1 = z0 - float(k1) + G3;
        float x2 = x0 - float(i2) + 2.0 * G3;
        float y2 = y0 - float(j2) + 2.0 * G3;
        float z2 = z0 - float(k2) + 2.0 * G3;
        float x3 = x0 - 1.0 + 3.0 * G3;
        float y3 = y0 - 1.0 + 3.0 * G3;
        float z3 = z0 - 1.0 + 3.0 * G3;

        int gi0 = int(get_perm(i + get_perm(j + get_perm(k))));
        int gi1 = int(get_perm(i + i1 + get_perm(j + j1 + get_perm(k + k1))));
        int gi2 = int(get_perm(i + i2 + get_perm(j + j2 + get_perm(k + k2))));
        int gi3 = int(get_perm(i + 1 + get_perm(j + 1 + get_perm(k + 1))));

        float t0 = 0.6 - dot(float3(x0, y0, z0), float3(x0, y0, z0));
        n0 = (t0 < 0.0) ? 0.0 : pow(t0, 4.0) * grad3D(gi0, x0, y0, z0);
        float t1 = 0.6 - dot(float3(x1, y1, z1), float3(x1, y1, z1));
        n1 = (t1 < 0.0) ? 0.0 : pow(t1, 4.0) * grad3D(gi1, x1, y1, z1);
        float t2 = 0.6 - dot(float3(x2, y2, z2), float3(x2, y2, z2));
        n2 = (t2 < 0.0) ? 0.0 : pow(t2, 4.0) * grad3D(gi2, x2, y2, z2);
        float t3 = 0.6 - dot(float3(x3, y3, z3), float3(x3, y3, z3));
        n3 = (t3 < 0.0) ? 0.0 : pow(t3, 4.0) * grad3D(gi3, x3, y3, z3);

        return 32.0f * (n0 + n1 + n2 + n3);
    }

    public float fractal<let octaves : int>(float3 coords, float frequency, float amplitude, float lacunarity, float persistence)
    {
        float sum = 0.0;
        float norm = 0.0;

        float3 pos = coords;

        for (int i = 0; i < octaves; i++)
        {
            sum += amplitude * simplex3D(pos * frequency);
            norm += amplitude;

            frequency *= lacunarity;
            amplitude *= persistence;
        }

        return sum / norm;
    }
};
