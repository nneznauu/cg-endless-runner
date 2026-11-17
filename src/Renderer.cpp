#include "Renderer.h"
#include "Shader.h"
#include "GlDiagnostics.h"

#include <glm/gtc/type_ptr.hpp>
#include <array>
#include <cctype>
#include <cstddef>
#include <cmath>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
    struct GroundVertex { float x, z; };
    struct SolidVertex { float x, y, z, nx, ny, nz; };
    struct TextVertex { float x, y; };

    void matrix(GLuint program, const char* name, const glm::mat4& value) {
        glUniformMatrix4fv(glGetUniformLocation(program, name), 1, GL_FALSE,
            glm::value_ptr(value));
    }
    void vector(GLuint program, const char* name, const glm::vec3& value) {
        glUniform3fv(glGetUniformLocation(program, name), 1, glm::value_ptr(value));
    }
    void scalar(GLuint program, const char* name, float value) {
        glUniform1f(glGetUniformLocation(program, name), value);
    }

    std::array<unsigned char, 7> glyph(char input) {
        const char c = static_cast<char>(std::toupper(static_cast<unsigned char>(input)));
        switch (c) {
        case 'A': return { 14,17,17,31,17,17,17 };
        case 'B': return { 30,17,17,30,17,17,30 };
        case 'C': return { 14,17,16,16,16,17,14 };
        case 'D': return { 30,17,17,17,17,17,30 };
        case 'E': return { 31,16,16,30,16,16,31 };
        case 'F': return { 31,16,16,30,16,16,16 };
        case 'G': return { 14,17,16,23,17,17,15 };
        case 'H': return { 17,17,17,31,17,17,17 };
        case 'I': return { 14,4,4,4,4,4,14 };
        case 'J': return { 7,2,2,2,2,18,12 };
        case 'K': return { 17,18,20,24,20,18,17 };
        case 'L': return { 16,16,16,16,16,16,31 };
        case 'M': return { 17,27,21,21,17,17,17 };
        case 'N': return { 17,25,25,21,19,19,17 };
        case 'O': return { 14,17,17,17,17,17,14 };
        case 'P': return { 30,17,17,30,16,16,16 };
        case 'Q': return { 14,17,17,17,21,18,13 };
        case 'R': return { 30,17,17,30,20,18,17 };
        case 'S': return { 15,16,16,14,1,1,30 };
        case 'T': return { 31,4,4,4,4,4,4 };
        case 'U': return { 17,17,17,17,17,17,14 };
        case 'V': return { 17,17,17,17,17,10,4 };
        case 'W': return { 17,17,17,21,21,21,10 };
        case 'X': return { 17,17,10,4,10,17,17 };
        case 'Y': return { 17,17,10,4,4,4,4 };
        case 'Z': return { 31,1,2,4,8,16,31 };
        case '0': return { 14,17,19,21,25,17,14 };
        case '1': return { 4,12,4,4,4,4,14 };
        case '2': return { 14,17,1,2,4,8,31 };
        case '3': return { 30,1,1,14,1,1,30 };
        case '4': return { 2,6,10,18,31,2,2 };
        case '5': return { 31,16,16,30,1,1,30 };
        case '6': return { 14,16,16,30,17,17,14 };
        case '7': return { 31,1,2,4,8,8,8 };
        case '8': return { 14,17,17,14,17,17,14 };
        case '9': return { 14,17,17,15,1,1,14 };
        case ':': return { 0,4,4,0,4,4,0 };
        case '.': return { 0,0,0,0,0,4,4 };
        case '-': return { 0,0,0,31,0,0,0 };
        case '/': return { 1,2,2,4,8,8,16 };
        case '|': return { 4,4,4,4,4,4,4 };
        case '(': return { 2,4,8,8,8,4,2 };
        case ')': return { 8,4,2,2,2,4,8 };
        case '+': return { 0,4,4,31,4,4,0 };
        case '!': return { 4,4,4,4,4,0,4 };
        default: return {};
        }
    }

    void appendText(std::vector<TextVertex>& vertices, const std::string& line,
        float x, float y, float pixel) {
        for (char c : line) {
            const auto rows = glyph(c);
            for (int row = 0; row < 7; ++row) {
                for (int col = 0; col < 5; ++col) {
                    if ((rows[row] & (1 << (4 - col))) == 0) continue;
                    const float left = x + col * pixel, top = y + row * pixel;
                    const float right = left + pixel, bottom = top + pixel;
                    vertices.insert(vertices.end(), {
                        {left,top},{right,top},{right,bottom},
                        {left,top},{right,bottom},{left,bottom}
                    });
                }
            }
            x += pixel * 6.0f;
        }
    }

    float textWidth(const std::string& line, float pixel) {
        return line.empty() ? 0.0f : (static_cast<float>(line.size()) * 6.0f - 1.0f) * pixel;
    }
}

void Renderer::initialize(const std::filesystem::path& root, const HeightField& terrain) {
    checkGraphics("application initialization entry");
    terrainProgram = loadProgram(root / "shaders/terrain.vert", root / "shaders/lit.frag", "worldPosition");
    playerProgram = loadProgram(root / "shaders/player.vert", root / "shaders/lit.frag");
    hudProgram = loadProgram(root / "shaders/hud.vert", root / "shaders/hud.frag");
    obstacleProgram = loadProgram(root / "shaders/obstacles.vert", root / "shaders/obstacles.frag");
    obstacles.initialize();
    checkGraphics("shader program creation");

    std::vector<GroundVertex> vertices;
    std::vector<unsigned int> indices;
    const int columns = static_cast<int>(HeightField::width / HeightField::gridStep) + 1;
    const int rows = static_cast<int>((HeightField::nearZ - HeightField::farZ)
        / HeightField::gridStep) + 1;
    for (int z = 0; z < rows; ++z) {
        for (int x = 0; x < columns; ++x) {
            vertices.push_back({ -HeightField::width * 0.5f + x * HeightField::gridStep,
                HeightField::farZ + z * HeightField::gridStep });
        }
    }
    for (int z = 0; z < rows - 1; ++z) {
        for (int x = 0; x < columns - 1; ++x) {
            const unsigned int a = static_cast<unsigned int>(z * columns + x);
            const unsigned int b = a + 1, c = a + columns, d = c + 1;
            indices.insert(indices.end(), { a,c,b, b,c,d });
        }
    }
    terrainIndexCount = static_cast<GLsizei>(indices.size());
    terrainVertexCount = static_cast<GLsizei>(vertices.size());
    glGenVertexArrays(1, &terrainVao);
    glGenBuffers(1, &terrainVbo);
    glGenBuffers(1, &terrainEbo);
    glBindVertexArray(terrainVao);
    glBindBuffer(GL_ARRAY_BUFFER, terrainVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(GroundVertex)),
        vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, terrainEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)),
        indices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(GroundVertex), nullptr);
    glEnableVertexAttribArray(0);

    checkGraphics("terrain mesh buffers");

    glGenTextures(1, &heightTexture);
    glBindTexture(GL_TEXTURE_2D, heightTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, HeightField::textureSize, HeightField::textureSize,
        0, GL_RED, GL_FLOAT, terrain.texels().data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    checkGraphics("height-map texture creation");

    const std::array<std::array<glm::vec3, 4>, 6> faces = {{
        {{ {.5f,-.5f,-.5f},{.5f,.5f,-.5f},{.5f,.5f,.5f},{.5f,-.5f,.5f} }},
        {{ {-.5f,-.5f,.5f},{-.5f,.5f,.5f},{-.5f,.5f,-.5f},{-.5f,-.5f,-.5f} }},
        {{ {-.5f,.5f,-.5f},{-.5f,.5f,.5f},{.5f,.5f,.5f},{.5f,.5f,-.5f} }},
        {{ {-.5f,-.5f,.5f},{-.5f,-.5f,-.5f},{.5f,-.5f,-.5f},{.5f,-.5f,.5f} }},
        {{ {-.5f,-.5f,.5f},{.5f,-.5f,.5f},{.5f,.5f,.5f},{-.5f,.5f,.5f} }},
        {{ {.5f,-.5f,-.5f},{-.5f,-.5f,-.5f},{-.5f,.5f,-.5f},{.5f,.5f,-.5f} }}
    }};
    const std::array<glm::vec3, 6> normals = {{
        {1,0,0},{-1,0,0},{0,1,0},{0,-1,0},{0,0,1},{0,0,-1}
    }};
    std::vector<SolidVertex> cubeVertices;
    std::vector<unsigned int> cubeIndices;
    for (unsigned int face = 0; face < 6; ++face) {
        for (const auto& position : faces[face]) {
            const auto& n = normals[face];
            cubeVertices.push_back({ position.x,position.y,position.z,n.x,n.y,n.z });
        }
        const unsigned int a = face * 4;
        cubeIndices.insert(cubeIndices.end(), { a,a+1,a+2, a,a+2,a+3 });
    }
    glGenVertexArrays(1, &playerVao);
    glGenBuffers(1, &playerVbo);
    glGenBuffers(1, &playerEbo);
    glBindVertexArray(playerVao);
    glBindBuffer(GL_ARRAY_BUFFER, playerVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(cubeVertices.size() * sizeof(SolidVertex)),
        cubeVertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, playerEbo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(cubeIndices.size() * sizeof(unsigned int)),
        cubeIndices.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(SolidVertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(SolidVertex),
        reinterpret_cast<void*>(offsetof(SolidVertex, nx)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    checkGraphics("player mesh buffers");

    glGenVertexArrays(1, &hudVao);
    glGenBuffers(1, &hudVbo);
    glBindVertexArray(hudVao);
    glBindBuffer(GL_ARRAY_BUFFER, hudVbo);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(TextVertex), nullptr);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);
    checkGraphics("HUD buffers and depth/culling state");
}

void Renderer::draw(int width, int height, const Camera& camera, const Player& player,
    float phase, bool displaced, float lightAngle, bool wireframe,
    const std::vector<std::string>& hud, bool instanced, bool showObstacles,
    const std::vector<std::string>& banner) {
    glClearColor(0.055f, 0.085f, 0.14f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    const glm::mat4 view = camera.view();
    const glm::mat4 projection = glm::perspective(glm::radians(60.0f),
        static_cast<float>(width) / height, 0.1f, 220.0f);
    const glm::vec3 light(std::sin(lightAngle), 1.25f, std::cos(lightAngle));

    glPolygonMode(GL_FRONT_AND_BACK, wireframe ? GL_LINE : GL_FILL);
    glUseProgram(terrainProgram);
    matrix(terrainProgram, "view", view);
    matrix(terrainProgram, "projection", projection);
    vector(terrainProgram, "cameraPosition", camera.eye);
    vector(terrainProgram, "directionToLight", light);
    glUniform1i(glGetUniformLocation(terrainProgram, "groundPass"), 1);
    scalar(terrainProgram, "heightScale", displaced ? HeightField::amplitude : 0.0f);
    scalar(terrainProgram, "terrainWidth", HeightField::width);
    scalar(terrainProgram, "terrainPeriod", HeightField::period);
    scalar(terrainProgram, "scrollPhase", phase);
    scalar(terrainProgram, "normalStep", HeightField::gridStep);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightTexture);
    glUniform1i(glGetUniformLocation(terrainProgram, "heightMap"), 0);
    glBindVertexArray(terrainVao);
    glDrawElements(GL_TRIANGLES, terrainIndexCount, GL_UNSIGNED_INT, nullptr);

    glUseProgram(playerProgram);
    matrix(playerProgram, "view", view);
    matrix(playerProgram, "projection", projection);
    vector(playerProgram, "cameraPosition", camera.eye);
    vector(playerProgram, "directionToLight", light);
    glUniform1i(glGetUniformLocation(playerProgram, "groundPass"), 0);
    glBindVertexArray(playerVao);
    const glm::mat4 base = glm::translate(glm::mat4(1.0f), glm::vec3(player.x, player.feetY, 0.0f));
    const auto part = [&](const glm::mat4& transform, const glm::vec3& size, const glm::vec3& color) {
        matrix(playerProgram, "model", glm::scale(transform, size));
        vector(playerProgram, "objectColor", color);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, nullptr);
    };
    const float legLength = Player::height * 0.42f;
    const float torsoTop = Player::height * 0.77f;
    const float stride = 0.6f * std::sin(glm::radians(360.0f * 48.0f / 128.0f) * phase);
    const auto leg = [&](float side, float angle) {
        glm::mat4 transform = glm::translate(base, glm::vec3(side, legLength, 0.0f));
        transform = glm::rotate(transform, angle, glm::vec3(1.0f, 0.0f, 0.0f));
        transform = glm::translate(transform, glm::vec3(0.0f, -legLength * 0.5f, 0.0f));
        part(transform, glm::vec3(0.22f, legLength, 0.24f), glm::vec3(0.15f, 0.17f, 0.30f));
    };
    leg(-0.18f, player.grounded ? stride : 0.3f);
    leg(0.18f, player.grounded ? -stride : 0.3f);
    part(glm::translate(base, glm::vec3(0.0f, (legLength + torsoTop) * 0.5f, 0.0f)),
        glm::vec3(0.6f, torsoTop - legLength, 0.36f), glm::vec3(0.20f, 0.35f, 0.85f));
    part(glm::translate(base, glm::vec3(0.0f, (torsoTop + Player::height) * 0.5f, 0.0f)),
        glm::vec3(0.3f, Player::height - torsoTop, 0.3f), glm::vec3(0.97f, 0.80f, 0.65f));
    glUseProgram(obstacleProgram);
    matrix(obstacleProgram, "uViewProjection", projection * view);
    vector(obstacleProgram, "uEye", camera.eye);
    vector(obstacleProgram, "uLightDirection", light);
    glUniform1i(glGetUniformLocation(obstacleProgram, "uPatterns"), obstaclePatterns ? 1 : 0);
    obstacleCalls = showObstacles ? obstacles.draw(obstacleProgram, instanced) : 0;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    if (!hud.empty() || !banner.empty()) drawText(width, height, hud, banner);
    glBindVertexArray(0);
}

void Renderer::drawText(int width, int height, const std::vector<std::string>& lines,
    const std::vector<std::string>& banner) {
    const bool compact = width < 900;
    const float pixel = compact ? 1.5f : 2.0f;
    std::vector<TextVertex> vertices;
    float y = 18.0f;
    for (const std::string& line : lines) {
        appendText(vertices, line, 18.0f, y, pixel);
        y += pixel * 11.0f;
    }
    if (!banner.empty()) {
        const float title = compact ? 6.0f : 8.0f, body = compact ? 2.5f : 3.5f;
        float total = title * 7.0f;
        for (std::size_t i = 1; i < banner.size(); ++i) total += body * 11.0f + (i == 1 ? title * 4.0f : 0.0f);
        float top = (static_cast<float>(height) - total) * 0.5f;
        for (std::size_t i = 0; i < banner.size(); ++i) {
            const float size = i == 0 ? title : body;
            appendText(vertices, banner[i], (static_cast<float>(width) - textWidth(banner[i], size)) * 0.5f, top, size);
            top += i == 0 ? title * 11.0f : body * 11.0f;
        }
    }
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glUseProgram(hudProgram);
    glUniform2f(glGetUniformLocation(hudProgram, "screenSize"),
        static_cast<float>(width), static_cast<float>(height));
    glBindVertexArray(hudVao);
    glBindBuffer(GL_ARRAY_BUFFER, hudVbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(TextVertex)),
        vertices.data(), GL_STREAM_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vertices.size()));
    glEnable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::updateObstacles(const runner::Game& game, const HeightField& terrain, bool displaced) {
    obstaclePatterns = !game.stress();
    std::vector<runner::Obstacle> visible;
    for (const auto& o : game.obstacles()) {
        if (runner::Game::onTerrain(o)) visible.push_back(o);
    }
    obstacles.upload(visible, [&](float x, float z) {
        return terrain.surfaceHeight(x, z, static_cast<float>(game.groundPhase()), displaced);
    });
}

void Renderer::validateTerrainSampling(const HeightField& terrain, float phase, bool displaced) {
    glUseProgram(terrainProgram);
    scalar(terrainProgram, "heightScale", displaced ? HeightField::amplitude : 0.0f);
    scalar(terrainProgram, "terrainWidth", HeightField::width);
    scalar(terrainProgram, "terrainPeriod", HeightField::period);
    scalar(terrainProgram, "scrollPhase", phase);
    scalar(terrainProgram, "normalStep", HeightField::gridStep);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, heightTexture);
    glUniform1i(glGetUniformLocation(terrainProgram, "heightMap"), 0);
    GLuint feedback = 0;
    glGenBuffers(1, &feedback);
    glBindBuffer(GL_TRANSFORM_FEEDBACK_BUFFER, feedback);
    std::vector<float> positions(static_cast<std::size_t>(terrainVertexCount) * 3);
    glBufferData(GL_TRANSFORM_FEEDBACK_BUFFER,
        static_cast<GLsizeiptr>(positions.size() * sizeof(float)), nullptr, GL_STREAM_READ);
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, feedback);
    glBindVertexArray(terrainVao);
    glEnable(GL_RASTERIZER_DISCARD);
    glBeginTransformFeedback(GL_POINTS);
    glDrawArrays(GL_POINTS, 0, terrainVertexCount);
    glEndTransformFeedback();
    glDisable(GL_RASTERIZER_DISCARD);
    glGetBufferSubData(GL_TRANSFORM_FEEDBACK_BUFFER, 0,
        static_cast<GLsizeiptr>(positions.size() * sizeof(float)), positions.data());
    glBindBufferBase(GL_TRANSFORM_FEEDBACK_BUFFER, 0, 0);
    glDeleteBuffers(1, &feedback);
    checkGraphics("terrain transform-feedback validation");
    const std::size_t columns = static_cast<std::size_t>(HeightField::width / HeightField::gridStep) + 1;
    constexpr float tolerance = 0.0001f;
    float maximumError = 0.0f;
    for (std::size_t i = 0; i < positions.size(); i += 3) {
        const std::size_t vertex = i / 3;
        const float x = -HeightField::width * 0.5f + static_cast<float>(vertex % columns) * HeightField::gridStep;
        const float z = HeightField::farZ + static_cast<float>(vertex / columns) * HeightField::gridStep;
        const float expected = terrain.sample(x, z, phase, displaced);
        const float error = std::abs(positions[i+1] - expected);
        if (!std::isfinite(positions[i]) || !std::isfinite(positions[i+1]) || !std::isfinite(positions[i+2])
            || std::abs(positions[i] - x) > tolerance || std::abs(positions[i+2] - z) > tolerance
            || error > tolerance) {
            std::ostringstream message;
            message << std::setprecision(9) << "Terrain CPU/GPU height mismatch: vertex=" << vertex
                << " phase=" << phase << " displaced=" << displaced
                << " expected_xyz=(" << x << ',' << expected << ',' << z << ')'
                << " gpu_xyz=(" << positions[i] << ',' << positions[i+1] << ',' << positions[i+2] << ')'
                << " height_error=" << error << " tolerance=" << tolerance;
            throw std::runtime_error(message.str());
        }
        if (error > maximumError) maximumError = error;
    }
    std::cout << "PASS terrain sampling: phase=" << phase << " displaced=" << displaced
        << " vertices=" << terrainVertexCount << " max_height_error=" << maximumError
        << " tolerance=" << tolerance << '\n';
}

void Renderer::shutdown() {
    obstacles.shutdown();
    glDeleteProgram(obstacleProgram);
    obstacleProgram = 0;
    const GLuint buffers[] = { terrainVbo,terrainEbo,playerVbo,playerEbo,hudVbo };
    const GLuint arrays[] = { terrainVao,playerVao,hudVao };
    glDeleteBuffers(5, buffers);
    glDeleteVertexArrays(3, arrays);
    glDeleteTextures(1, &heightTexture);
    glDeleteProgram(terrainProgram);
    glDeleteProgram(playerProgram);
    glDeleteProgram(hudProgram);
    terrainProgram = playerProgram = hudProgram = 0;
    terrainVbo = terrainEbo = playerVbo = playerEbo = hudVbo = 0;
    terrainVao = playerVao = hudVao = heightTexture = 0;
}
