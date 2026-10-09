#include "Game.h"
#include <cmath>
#include <stdexcept>

namespace runner {
Game::Game(std::size_t count) {
    if (count == 0 || count > 10000)
        throw std::invalid_argument("Obstacle count must be in [1, 10000]");
    obstacles_.resize(count);
    positions_.resize(count);
    loopLength_ = 6.0 * static_cast<double>(count) + 30.0;
    reset();
}
void Game::reset() {
    state_ = RunState::Ready;
    distance_ = groundPhase_ = 0;
    recycled_ = 0;
    for (std::size_t i = 0; i < obstacles_.size(); ++i) {
        positions_[i] = -18.0 - 6.0 * static_cast<double>(i);
        const int lane = static_cast<int>((i * 7 + 1) % 3) - 1;
        obstacles_[i] = {3.2f * lane, static_cast<float>(positions_[i]),
                         1.55f, 1.2f + 0.35f * static_cast<float>(i % 3), 1.35f};
    }
}
void Game::startOrResume() { state_ = RunState::Running; }
void Game::togglePause() {
    if (state_ == RunState::Running) state_ = RunState::Paused;
    else if (state_ == RunState::Paused) state_ = RunState::Running;
}
void Game::update(double dt) {
    if (!std::isfinite(dt) || dt < 0.0 || dt > 3600.0)
        throw std::invalid_argument("dt must be finite and in [0, 3600] seconds");
    if (state_ != RunState::Running) return;
    const double travel = speed() * dt;
    distance_ += travel;
    groundPhase_ = std::fmod(groundPhase_ + travel, GroundPeriod);
    for (std::size_t i = 0; i < positions_.size(); ++i) {
        double z = positions_[i] + travel;
        if (z > RecycleZ) {
            const double wraps = std::ceil((z - RecycleZ) / loopLength_);
            z -= wraps * loopLength_;
            recycled_ += static_cast<std::uint64_t>(wraps);
        }
        positions_[i] = z;
        obstacles_[i].z = static_cast<float>(z);
    }
}
const char* stateName(RunState state) {
    switch (state) {
    case RunState::Ready: return "READY";
    case RunState::Running: return "RUNNING";
    case RunState::Paused: return "PAUSED";
    }
    return "UNKNOWN";
}
}
