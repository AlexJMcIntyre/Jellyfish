#include "jell_field.hpp"

#include <cmath>

namespace
{
    // Linear interpolation
    float lerp(float a, float b, float t)
    {
        return a + t * (b - a);
    }

    // Smooth interpolation curve
    float fade(float t)
    {
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }

    // Integer hash -> float [0,1]
    float hash(int x, int y, int z)
    {
        uint32_t h =
            (uint32_t)x * 374761393u +
            (uint32_t)y * 668265263u +
            (uint32_t)z * 2147483647u;

        h = (h ^ (h >> 13)) * 1274126177u;
        h ^= h >> 16;

        return h / 4294967295.0f;
    }
}

float Field::noise(
    Point3 p,
    float scale,
    float time)
{
    // Scale the sample position
    float x = p.x * scale + time;
    float y = p.y * scale;
    float z = p.z * scale;

    // Find containing cube
    int x0 = (int)floorf(x);
    int y0 = (int)floorf(y);
    int z0 = (int)floorf(z);

    int x1 = x0 + 1;
    int y1 = y0 + 1;
    int z1 = z0 + 1;

    // Position within the cube
    float tx = fade(x - x0);
    float ty = fade(y - y0);
    float tz = fade(z - z0);

    // Sample all eight cube corners
    float v000 = hash(x0, y0, z0);
    float v100 = hash(x1, y0, z0);
    float v010 = hash(x0, y1, z0);
    float v110 = hash(x1, y1, z0);

    float v001 = hash(x0, y0, z1);
    float v101 = hash(x1, y0, z1);
    float v011 = hash(x0, y1, z1);
    float v111 = hash(x1, y1, z1);

    // Interpolate along X
    float i00 = lerp(v000, v100, tx);
    float i10 = lerp(v010, v110, tx);

    float i01 = lerp(v001, v101, tx);
    float i11 = lerp(v011, v111, tx);

    // Interpolate along Y
    float j0 = lerp(i00, i10, ty);
    float j1 = lerp(i01, i11, ty);

    // Interpolate along Z
    return lerp(j0, j1, tz);
}