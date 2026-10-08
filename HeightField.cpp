#include "HeightField.h"

#include <algorithm>
#include <cmath>

namespace {
    constexpr float pi = 3.14159265358979323846f;
    float blend(float a, float b, float t) { return a + (b - a) * t; }
}

HeightField::HeightField() : values(textureSize * textureSize) {
    for (int z = 0; z < textureSize; ++z) {
        for (int x = 0; x < textureSize; ++x) {
            // Sampling at texel centres matches OpenGL's linear filtering.
            const float u = (x + 0.5f) / textureSize;
            const float v = (z + 0.5f) / textureSize;
            const float edge = (2.0f * u - 1.0f) * (2.0f * u - 1.0f);
            const float rollingRoad = 0.28f + 0.13f * std::sin(2.0f * pi * v)
                + 0.08f * std::cos(6.0f * pi * v);
            const float roadside = edge * (0.22f
                + 0.09f * std::sin(4.0f * pi * u + 2.0f * pi * v));
            values[z * textureSize + x] = rollingRoad + roadside;
        }
    }
}

float HeightField::texel(int x, int z) const {
    x = std::clamp(x, 0, textureSize - 1);
    z = (z % textureSize + textureSize) % textureSize;
    return values[z * textureSize + x];
}

float HeightField::sample(float x, float z, float phase, bool displaced) const {
    if (!displaced) return 0.0f;
    const float u = std::clamp(x / width + 0.5f, 0.0f, 1.0f);
    float v = (z - phase) / period;
    v -= std::floor(v);
    const float tx = u * textureSize - 0.5f;
    const float tz = v * textureSize - 0.5f;
    const int ix = static_cast<int>(std::floor(tx));
    const int iz = static_cast<int>(std::floor(tz));
    const float fx = tx - std::floor(tx);
    const float fz = tz - std::floor(tz);
    const float lower = blend(texel(ix, iz), texel(ix + 1, iz), fx);
    const float upper = blend(texel(ix, iz + 1), texel(ix + 1, iz + 1), fx);
    return amplitude * blend(lower, upper, fz);
}

float HeightField::surfaceHeight(float x, float z, float phase, bool displaced) const {
    // Match the actual two triangles in each rendered grid cell.
    const float gx = std::clamp(x, -width * 0.5f, width * 0.5f - 0.0001f);
    const float gz = std::clamp(z, farZ, nearZ - 0.0001f);
    const float cellX = std::floor((gx + width * 0.5f) / gridStep);
    const float cellZ = std::floor((gz - farZ) / gridStep);
    const float x0 = -width * 0.5f + cellX * gridStep;
    const float z0 = farZ + cellZ * gridStep;
    const float fx = (gx - x0) / gridStep;
    const float fz = (gz - z0) / gridStep;
    const float a = sample(x0, z0, phase, displaced);
    const float b = sample(x0 + gridStep, z0, phase, displaced);
    const float c = sample(x0, z0 + gridStep, phase, displaced);
    const float d = sample(x0 + gridStep, z0 + gridStep, phase, displaced);
    if (fx + fz <= 1.0f) return a + (b - a) * fx + (c - a) * fz;
    return d + (c - d) * (1.0f - fx) + (b - d) * (1.0f - fz);
}
