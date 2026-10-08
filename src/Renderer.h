#pragma once

#include "Camera.h"
#include "HeightField.h"
#include "Player.h"
#include "Obstacles.h"
#include <glad/glad.h>
#include <filesystem>
#include <string>
#include <vector>

class Renderer {
public:
    void initialize(const std::filesystem::path& root, const HeightField& terrain);
    void draw(int width, int height, const Camera& camera, const Player& player,
        float phase, bool displaced, float lightAngle, bool wireframe,
        const std::vector<std::string>& hud, bool instanced = true, bool showObstacles = true);
    void updateObstacles(const runner::Game& game, const HeightField& terrain, bool displaced);
    std::size_t obstacleCount() const { return obstacles.count(); }
    std::size_t obstacleDrawCalls() const { return obstacleCalls; }
    void validateInstanceLayout() const { obstacles.validateLayout(); }
    void validateTerrainSampling(const HeightField& terrain, float phase, bool displaced);
    void shutdown();

private:
    GLuint terrainProgram = 0, playerProgram = 0, hudProgram = 0;
    GLuint obstacleProgram = 0;
    runner::Obstacles obstacles;
    std::size_t obstacleCalls = 0;
    GLuint terrainVao = 0, terrainVbo = 0, terrainEbo = 0, heightTexture = 0;
    GLuint playerVao = 0, playerVbo = 0, playerEbo = 0;
    GLuint hudVao = 0, hudVbo = 0;
    GLsizei terrainIndexCount = 0;
    GLsizei terrainVertexCount = 0;
    void drawText(int width, int height, const std::vector<std::string>& lines);
};
