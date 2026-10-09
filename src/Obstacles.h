#pragma once
#include "Game.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <functional>
#include <vector>

namespace runner {
using GroundHeight = std::function<float(float, float)>;

class Obstacles {
public:
    Obstacles() = default;
    void initialize();
    void shutdown();
    Obstacles(const Obstacles&) = delete;
    Obstacles& operator=(const Obstacles&) = delete;
    void upload(const std::vector<Obstacle>& obstacles, const GroundHeight& height);
    std::size_t draw(GLuint program, bool instanced) const;
    std::size_t count() const { return transforms_.size(); }
    void validateLayout() const;
private:
    GLuint vao_ = 0, mesh_ = 0, indices_ = 0, instances_ = 0;
    std::vector<glm::mat4> transforms_;
};
}
