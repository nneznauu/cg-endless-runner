#pragma once

#include "HeightField.h"

class Player {
public:
    float x = 0.0f;
    float feetY = 0.0f;
    bool grounded = true;
    static constexpr float height = 1.2f;

    void reset(const HeightField& terrain, float phase, bool displaced);
    void jump();
    void update(float dt, float horizontalInput, const HeightField& terrain,
        float phase, bool displaced);

private:
    float verticalSpeed = 0.0f;
};
