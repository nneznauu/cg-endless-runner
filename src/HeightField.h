#pragma once

#include <vector>

// The CPU owns the height texture; the vertex shader displaces the mesh.
class HeightField {
public:
    static constexpr int textureSize = 128;
    static constexpr float width = 24.0f;
    static constexpr float period = 128.0f;
    static constexpr float amplitude = 2.5f;
    static constexpr float gridStep = 0.5f;
    static constexpr float nearZ = 16.0f;
    static constexpr float farZ = -160.0f;

    HeightField();
    const std::vector<float>& texels() const { return values; }
    float sample(float x, float z, float phase, bool displaced) const;
    float surfaceHeight(float x, float z, float phase, bool displaced) const;

private:
    std::vector<float> values;
    float texel(int x, int z) const;
};
