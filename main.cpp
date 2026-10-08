#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

#include <glad/glad.h>
#include <GL/freeglut.h>
#include "Camera.h"
#include "HeightField.h"
#include "Player.h"
#include "Renderer.h"
#include "GlDiagnostics.h"
#include "Keyboard.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
    HeightField terrain;
    Player player;
    Camera camera;
    Renderer renderer;
    int windowWidth = 1280, windowHeight = 720;
    bool held[256]{};
    bool arrows[4]{};
    bool paused = false, displaced = true, help = true, wireframe = false;
    bool graphicsReady = false;
    bool runtimeFailed = false;
    bool firstFrame = true;
    float phase = 0.0f, lightAngle = 0.8f;
    constexpr float runSpeed = 8.0f;
    double distanceTravelled = 0.0;
    double fpsSeconds = 0.0;
    int fpsFrames = 0, fps = 0;
    auto lastFrame = std::chrono::steady_clock::now();

    std::filesystem::path findRoot(const char* executable) {
        std::vector<std::filesystem::path> starts{ std::filesystem::current_path() };
#ifdef _WIN32
        wchar_t path[32768]{};
        const DWORD length = GetModuleFileNameW(nullptr, path, 32768);
        if (length != 0 && length < 32768) {
            starts.push_back(std::filesystem::path(path).parent_path());
        }
#else
        starts.push_back(std::filesystem::absolute(executable).parent_path());
#endif
        for (auto path : starts) {
            for (int level = 0; level < 5; ++level) {
                if (std::filesystem::exists(path / "shaders/terrain.vert")) return path;
                const auto parent = path.parent_path();
                if (parent == path) break;
                path = parent;
            }
        }
        throw std::runtime_error("Shader folder not found. Run from the project folder or rebuild.");
    }

    std::vector<std::string> statusText() {
        std::ostringstream score;
        score << "DISTANCE: " << std::fixed << std::setprecision(0) << distanceTravelled
            << " M | FPS: " << fps;
        std::vector<std::string> lines{
            "ENDLESS RUNNER | TOPIC 4 | STAGE 1",
            "MUKHAMEJAN / DANIYAR",
            score.str(),
            std::string("HEIGHT MAP: ") + (displaced ? "ON" : "OFF")
                + " | CAMERA: " + (camera.orbit ? "ORBIT" : "FOLLOW")
                + " | " + (paused ? "PAUSED" : "RUNNING")
        };
        if (help) {
            lines.push_back("SPACE: JUMP | A/D: MOVE | P: PAUSE | R: RESET");
            lines.push_back("T: HEIGHT MAP | C: CAMERA | V: WIREFRAME");
            lines.push_back("ARROWS: ORBIT | WHEEL: ZOOM | J/L: LIGHT");
            lines.push_back("H: HELP | ESC: EXIT");
        }
        else lines.push_back("H: HELP");
        return lines;
    }

    void display() {
        try {
            renderer.draw(windowWidth, windowHeight, camera, player, phase, displaced,
                lightAngle, wireframe, statusText());
#ifndef NDEBUG
            checkGraphics("frame rendering");
#else
            if (firstFrame) checkGraphics("first frame rendering");
#endif
            firstFrame = false;
            glutSwapBuffers();
            ++fpsFrames;
        }
        catch (const std::exception& error) {
            std::cerr << "ERROR: " << error.what() << '\n';
            runtimeFailed = true;
            glutLeaveMainLoop();
        }
    }

    void reshape(int width, int height) {
        windowWidth = std::max(width, 1);
        windowHeight = std::max(height, 1);
        glViewport(0, 0, windowWidth, windowHeight);
    }

    void performKeyAction(unsigned char key);

    void idle() {
#ifdef _WIN32
        if (!pollWindowsLetterKeys(held, performKeyAction)) {
            // A release sent to another window must not leave a game key held.
            std::fill_n(arrows, 4, false);
            held[' '] = false;
        }
#endif
        const auto now = std::chrono::steady_clock::now();
        const double actualDt = std::chrono::duration<double>(now - lastFrame).count();
        lastFrame = now;
        const float dt = static_cast<float>(std::clamp(actualDt, 0.0, 0.05));
        if (!paused) {
            distanceTravelled += runSpeed * dt;
            // Keep shader coordinates bounded, even after a very long run.
            phase = std::fmod(phase + runSpeed * dt, HeightField::period);
            const float move = (held['d'] ? 1.0f : 0.0f) - (held['a'] ? 1.0f : 0.0f);
            player.update(dt, move, terrain, phase, displaced);
        }
        lightAngle += ((held['l'] ? 1.0f : 0.0f) - (held['j'] ? 1.0f : 0.0f)) * dt;
        lightAngle = std::fmod(lightAngle, 6.283185307f);
        camera.update(dt, player, (arrows[1] ? 1.0f : 0.0f) - (arrows[0] ? 1.0f : 0.0f),
            (arrows[2] ? 1.0f : 0.0f) - (arrows[3] ? 1.0f : 0.0f),
            terrain.surfaceHeight(player.x, 0.0f, phase, displaced));
        fpsSeconds += actualDt;
        if (fpsSeconds >= 1.0) {
            fps = static_cast<int>(fpsFrames / fpsSeconds);
            fpsFrames = 0;
            fpsSeconds = 0.0;
        }
        glutPostRedisplay();
    }

    unsigned char normalizeKey(unsigned char key) {
        if (key >= 'A' && key <= 'Z') return static_cast<unsigned char>(key + ('a' - 'A'));
        return key;
    }

    void keyboard(unsigned char input, int, int) {
#ifdef _WIN32
        // Letters are read in idle() by virtual-key code, avoiding both
        // translated Cyrillic characters and duplicate English-key actions.
        if (input != ' ' && input != 27) return;
#endif
        const unsigned char key = normalizeKey(input);
        if (held[key]) return;
        held[key] = true;
        performKeyAction(key);
    }

    void performKeyAction(unsigned char key) {
        switch (key) {
        case 27: glutLeaveMainLoop(); break;
        case ' ': if (!paused) player.jump(); break;
        case 'p': paused = !paused; break;
        case 't':
            displaced = !displaced;
            player.update(0.0f, 0.0f, terrain, phase, displaced);
            break;
        case 'c': camera.orbit = !camera.orbit; break;
        case 'v': wireframe = !wireframe; break;
        case 'h': help = !help; break;
        case 'r':
            phase = 0.0f;
            distanceTravelled = 0.0;
            player.reset(terrain, phase, displaced);
            break;
        default: break;
        }
    }

    void keyboardUp(unsigned char key, int, int) {
#ifdef _WIN32
        if (key != ' ' && key != 27) return;
#endif
        held[normalizeKey(key)] = false;
    }
    int arrowIndex(int key) {
        switch (key) {
        case GLUT_KEY_LEFT: return 0;
        case GLUT_KEY_RIGHT: return 1;
        case GLUT_KEY_UP: return 2;
        case GLUT_KEY_DOWN: return 3;
        default: return -1;
        }
    }
    void special(int key, int, int) { const int i = arrowIndex(key); if (i >= 0) arrows[i] = true; }
    void specialUp(int key, int, int) { const int i = arrowIndex(key); if (i >= 0) arrows[i] = false; }
    void wheel(int, int direction, int, int) { if (camera.orbit) camera.zoom(direction); }
    void close() {
        if (graphicsReady) {
            renderer.shutdown();
            graphicsReady = false;
        }
    }
}

int main(int argc, char** argv) {
    try {
        glutInit(&argc, argv);
        glutInitContextVersion(3, 3);
        glutInitContextProfile(GLUT_CORE_PROFILE);
        glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH);
        glutInitWindowSize(windowWidth, windowHeight);
        glutCreateWindow("Topic 4 - Endless runner with instanced obstacles and a height-map ground | Stage 1");
        glutSetOption(GLUT_ACTION_ON_WINDOW_CLOSE, GLUT_ACTION_GLUTMAINLOOP_RETURNS);
        if (!gladLoadGL() || !GLAD_GL_VERSION_3_3) {
            throw std::runtime_error("OpenGL 3.3 core could not be initialized.");
        }
        std::cout << "OpenGL: " << glGetString(GL_VERSION) << '\n'
            << "Renderer: " << glGetString(GL_RENDERER) << '\n';
        reportStartupGraphicsErrors();
        graphicsReady = true;
        const auto root = findRoot(argv[0]);
        renderer.initialize(root, terrain);
        player.reset(terrain, phase, displaced);
        camera.update(0.0f, player, 0.0f, 0.0f,
            terrain.surfaceHeight(player.x, 0.0f, phase, displaced));
        reshape(windowWidth, windowHeight);
        for (const auto& line : statusText()) std::cout << line << '\n';
        std::cout << "Stage 1: terrain/player prototype. Obstacles and game-over are pending.\n";
        glutDisplayFunc(display);
        glutReshapeFunc(reshape);
        glutKeyboardFunc(keyboard);
        glutKeyboardUpFunc(keyboardUp);
        glutSpecialFunc(special);
        glutSpecialUpFunc(specialUp);
        glutMouseWheelFunc(wheel);
        glutCloseFunc(close);
        glutIgnoreKeyRepeat(1);
        glutIdleFunc(idle);
        lastFrame = std::chrono::steady_clock::now();
        glutMainLoop();
        // freeglut invokes close() while the context still exists.
        return runtimeFailed ? 1 : 0;
    }
    catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        if (graphicsReady && glutGetWindow() != 0) close();
        return 1;
    }
}
