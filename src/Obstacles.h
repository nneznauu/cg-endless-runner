#pragma once
#include "Game.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <array>
#include <vector>

namespace runner {
class Obstacles {
public:
    Obstacles() = default;
    void initialize();
    void shutdown();
    Obstacles(const Obstacles&) = delete;
    Obstacles& operator=(const Obstacles&) = delete;
    void upload(const std::vector<Obstacle>& obstacles, const GroundHeight& height);
    std::size_t draw(GLuint program, bool instanced) const;
    std::size_t count() const;
    std::size_t count(int type) const;
    std::size_t drawCalls(bool instanced) const;
    void validateLayout() const;
    static glm::vec3 color(int type);
    static constexpr GLuint ColorLocation = 6;
private:
    struct Instance {
        glm::mat4 model;
        glm::vec3 color;
    };
    struct Batch {
        GLuint vao = 0, buffer = 0;
        std::vector<Instance> instances;
    };
    GLuint mesh_ = 0, indices_ = 0;
    std::array<Batch, Game::TypeCount> batches_{};
};
}
