#pragma once
#include "Renderer.h"
#include <functional>

struct RunOptions {
    bool smoke = false, benchmark = false;
    std::size_t instances = 96;
    std::filesystem::path screenshot;
};
RunOptions parseRunOptions(int argc, char** argv);
void runValidation(const RunOptions& options, Renderer& renderer,
    HeightField& terrain, Player& player, Camera& camera, runner::Game& game,
    bool& displaced, bool& instanced,
    const std::function<void(float,float)>& tick,
    const std::function<void(unsigned char)>& key,
    const std::function<std::vector<std::string>()>& hud);
