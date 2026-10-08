#pragma once
#include "HeightField.h"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace runner {
enum class RunState { Ready, Running, Paused };

// Local right-handed coordinates: +Y up, -Z forward, +X right.
// Ground contact is the bottom of each axis-aligned obstacle.
struct Obstacle {
    float x, z;
    float width, height, depth;
};

// Daniyar: simulation, bounded obstacle coordinates and distance score.
// No OpenGL dependency; the teammate's player/terrain can consume this state.
class Game {
public:
    explicit Game(std::size_t count = 96);
    void reset();
    void startOrResume();
    void togglePause();
    void update(double dt);
    RunState state() const { return state_; }
    double distance() const { return distance_; }
    double groundPhase() const { return groundPhase_; }
    double speed() const { return 8.0; }
    double loopLength() const { return loopLength_; }
    std::uint64_t recycled() const { return recycled_; }
    const std::vector<Obstacle>& obstacles() const { return obstacles_; }
    static constexpr double GroundPeriod = HeightField::period;
    static constexpr double RecycleZ = 12.0;
private:
    std::vector<Obstacle> obstacles_;
    // Doubles retain sub-frame movement; floats are produced only for rendering.
    std::vector<double> positions_;
    RunState state_ = RunState::Ready;
    double distance_ = 0, groundPhase_ = 0, loopLength_ = 0;
    std::uint64_t recycled_ = 0;
};
const char* stateName(RunState state);
}
