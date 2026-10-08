#include "Player.h"

#include <algorithm>

void Player::reset(const HeightField& terrain, float phase, bool displaced) {
    x = 0.0f;
    verticalSpeed = 0.0f;
    grounded = true;
    feetY = terrain.surfaceHeight(x, 0.0f, phase, displaced);
}

void Player::jump() {
    if (grounded) {
        grounded = false;
        verticalSpeed = 7.2f;
    }
}

void Player::update(float dt, float horizontalInput, const HeightField& terrain,
    float phase, bool displaced) {
    x = std::clamp(x + horizontalInput * 4.8f * dt, -4.2f, 4.2f);
    const float ground = terrain.surfaceHeight(x, 0.0f, phase, displaced);
    if (grounded) {
        feetY = ground;
        return;
    }
    verticalSpeed -= 18.0f * dt;
    feetY += verticalSpeed * dt;
    if (feetY <= ground) {
        feetY = ground;
        verticalSpeed = 0.0f;
        grounded = true;
    }
}
