#pragma once

#include "Player.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

class Camera {
public:
    bool orbit = false;
    glm::vec3 eye{ 0.0f, 4.0f, 8.0f };
    glm::vec3 target{ 0.0f, 1.0f, -14.0f };

    void update(float dt, const Player& player, float turn, float tilt, float groundY) {
        if (orbit) {
            yaw += turn * dt;
            pitch = std::clamp(pitch + tilt * dt * 0.7f, 0.15f, 1.25f);
            target = glm::vec3(0.0f, groundY + 0.6f, -4.0f);
            eye = target + radius * glm::vec3(
                std::sin(yaw) * std::cos(pitch), std::sin(pitch),
                std::cos(yaw) * std::cos(pitch));
        }
        else {
            eye = glm::vec3(player.x * 0.3f, groundY + 3.7f, 8.0f);
            target = glm::vec3(player.x * 0.3f, groundY + 0.7f, -14.0f);
        }
    }
    void zoom(int direction) { radius = std::clamp(radius - direction, 4.0f, 28.0f); }
    glm::mat4 view() const { return glm::lookAt(eye, target, glm::vec3(0, 1, 0)); }

private:
    float yaw = 0.0f;
    float pitch = 0.40f;
    float radius = 13.0f;
};
