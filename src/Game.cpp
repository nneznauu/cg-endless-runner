#include "Game.h"
#include "Player.h"
#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace runner {
namespace {
std::uint64_t mix(std::uint64_t value) {
    value += 0x9E3779B97F4A7C15ull;
    value = (value ^ (value >> 30)) * 0xBF58476D1CE4E5B9ull;
    value = (value ^ (value >> 27)) * 0x94D049BB133111EBull;
    return value ^ (value >> 31);
}
std::uint64_t layoutHash(std::uint64_t serial, std::uint64_t salt) {
    return mix(Game::LayoutSeed ^ mix(serial * 8 + salt));
}
double layoutUnit(std::uint64_t serial, std::uint64_t salt) {
    return static_cast<double>(layoutHash(serial, salt) >> 11) * (1.0 / 9007199254740992.0);
}
int pickType(double unit) {
    if (unit < 0.4) return 0;
    if (unit < 0.75) return 1;
    return 2;
}
double difficulty(std::uint64_t serial) {
    const double arrival = Game::FirstRowDistance + Game::RowSpacing * static_cast<double>(serial);
    return (Game::speedAtDistance(arrival) - Game::BaseSpeed) / (Game::MaxSpeed - Game::BaseSpeed);
}
double repeatChance(std::uint64_t serial) {
    return 0.1 + 0.75 * difficulty(serial);
}
}

Game::Game(std::size_t count, bool stress) {
    configure(count, stress);
}
void Game::configure(std::size_t count, bool stress) {
    if (count == 0 || count > 10000)
        throw std::invalid_argument("Obstacle count must be in [1, 10000]");
    if (!stress_) best_ = std::max(best_, distance_);
    distance_ = 0;
    stress_ = stress;
    poolSize_ = count;
    const std::size_t slots = stress ? count : std::max<std::size_t>(1, (count + 1) / 2);
    positions_.assign(slots, 0.0);
    serials_.assign(slots, 0);
    layouts_.assign(slots, RowLayout{});
    obstacles_.clear();
    obstacles_.reserve(stress ? count : slots * 2);
    double nearest = 0, span = 0;
    stressBounds(nearest, span);
    loopLength_ = stress ? span : RowSpacing * static_cast<double>(slots);
    if (stress) restart();
    else reset();
}
void Game::stressBounds(double& nearest, double& span) {
    nearest = HeightField::nearZ - StressSize * 0.5 - 0.01;
    span = nearest - (HeightField::farZ + StressSize * 0.5 + 0.01);
}
void Game::reset() {
    if (!stress_) best_ = std::max(best_, distance_);
    state_ = RunState::Ready;
    distance_ = elapsed_ = groundPhase_ = 0;
    recycled_ = 0;
    if (stress_) {
        obstacles_.clear();
        for (std::size_t i = 0; i < positions_.size(); ++i) {
            obstacles_.push_back(makeStressObstacle(i));
            positions_[i] = obstacles_[i].z;
        }
        return;
    }
    for (std::size_t i = 0; i < positions_.size(); ++i) {
        serials_[i] = i;
        positions_[i] = -FirstRowDistance - RowSpacing * static_cast<double>(i);
        layouts_[i] = rowLayout(i);
    }
    rebuild();
}
void Game::restart() {
    reset();
    state_ = RunState::Running;
}
void Game::startOrResume() {
    if (state_ == RunState::Ready || state_ == RunState::Paused) state_ = RunState::Running;
}
void Game::togglePause() {
    if (state_ == RunState::Running) state_ = RunState::Paused;
    else if (state_ == RunState::Paused) state_ = RunState::Running;
}
double Game::speedAt(double seconds) {
    return std::min(MaxSpeed, BaseSpeed + Acceleration * std::max(seconds, 0.0));
}
double Game::distanceAt(double seconds) {
    const double t = std::max(seconds, 0.0);
    const double rampEnd = (MaxSpeed - BaseSpeed) / Acceleration;
    if (t <= rampEnd) return BaseSpeed * t + 0.5 * Acceleration * t * t;
    const double rampDistance = BaseSpeed * rampEnd + 0.5 * Acceleration * rampEnd * rampEnd;
    return rampDistance + MaxSpeed * (t - rampEnd);
}
double Game::speedAtDistance(double metres) {
    return std::min(MaxSpeed, std::sqrt(BaseSpeed * BaseSpeed + 2.0 * Acceleration * std::max(metres, 0.0)));
}
double Game::doubleRowChance(std::uint64_t serial) {
    const double repeat = repeatChance(serial + 1);
    return repeat + (1.0 - repeat) / 3.0;
}
int Game::baseLane(std::uint64_t serial) {
    std::uint64_t source = serial - serial % 2;
    while (source >= 2 && layoutUnit(source, 1) < repeatChance(source)) source -= 2;
    return static_cast<int>(layoutHash(source, 0) % 3);
}
RowLayout Game::rowLayout(std::uint64_t serial) {
    RowLayout row{1, {baseLane(serial), 0}, {pickType(layoutUnit(serial, 3)), 0}};
    if (serial % 2 == 0) return row;
    const int previous = baseLane(serial - 1), next = baseLane(serial + 1);
    if (previous != next) {
        row.lane[0] = 3 - previous - next;
        return row;
    }
    row.count = 0;
    for (int lane = 0; lane < 3; ++lane) {
        if (lane == previous) continue;
        row.lane[row.count] = lane;
        row.type[row.count] = pickType(layoutUnit(serial, 3 + static_cast<std::uint64_t>(row.count)));
        ++row.count;
    }
    return row;
}
Obstacle Game::makeObstacle(int lane, int type, float z) {
    static constexpr float heights[TypeCount]{0.75f, 1.6f, 3.4f};
    static constexpr float depths[TypeCount]{0.6f, 1.6f, 1.0f};
    const int t = std::clamp(type, 0, TypeCount - 1);
    return {laneX(lane), z, ObstacleWidth, heights[t], depths[t], t};
}
Obstacle Game::makeStressObstacle(std::size_t index) {
    static constexpr float heights[TypeCount]{0.5f, 1.0f, 1.8f};
    const int type = static_cast<int>(index % TypeCount);
    double nearest = 0, span = 0;
    stressBounds(nearest, span);
    const auto serial = static_cast<std::uint64_t>(index) + 0x5000000000ull;
    const float x = StressHalfWidth * static_cast<float>(2.0 * layoutUnit(serial, 6) - 1.0);
    const float z = static_cast<float>(nearest - span * layoutUnit(serial, 7));
    return {x, z, StressSize, heights[type], StressSize, type};
}
std::size_t Game::nextStressCount(std::size_t count) {
    for (std::size_t step : StressSteps) if (step > count) return step;
    return StressSteps.back();
}
std::size_t Game::previousStressCount(std::size_t count) {
    for (auto it = StressSteps.rbegin(); it != StressSteps.rend(); ++it) if (*it < count) return *it;
    return StressSteps.front();
}
void Game::rebuild() {
    obstacles_.clear();
    for (std::size_t i = 0; i < positions_.size(); ++i) {
        const RowLayout& row = layouts_[i];
        for (int k = 0; k < row.count; ++k)
            obstacles_.push_back(makeObstacle(row.lane[k], row.type[k], static_cast<float>(positions_[i])));
    }
}
void Game::update(double dt) {
    if (!std::isfinite(dt) || dt < 0.0 || dt > 3600.0)
        throw std::invalid_argument("dt must be finite and in [0, 3600] seconds");
    if (state_ != RunState::Running) return;
    elapsed_ += dt;
    const double next = distanceAt(elapsed_);
    const double travel = next - distance_;
    distance_ = next;
    groundPhase_ = std::fmod(groundPhase_ + travel, GroundPeriod);
    if (stress_) {
        double nearest = 0, span = 0;
        stressBounds(nearest, span);
        for (std::size_t i = 0; i < positions_.size(); ++i) {
            double z = positions_[i] + travel;
            if (z > nearest) {
                const double wraps = std::ceil((z - nearest) / span);
                z -= wraps * span;
                recycled_ += static_cast<std::uint64_t>(wraps);
            }
            positions_[i] = z;
            obstacles_[i].z = static_cast<float>(z);
        }
        return;
    }
    const auto rows = static_cast<std::uint64_t>(positions_.size());
    for (std::size_t i = 0; i < positions_.size(); ++i) {
        double z = positions_[i] + travel;
        if (z > RecycleZ) {
            const double wraps = std::ceil((z - RecycleZ) / loopLength_);
            z -= wraps * loopLength_;
            const auto count = static_cast<std::uint64_t>(wraps);
            recycled_ += count;
            serials_[i] += count * rows;
            layouts_[i] = rowLayout(serials_[i]);
        }
        positions_[i] = z;
    }
    rebuild();
}
bool Game::onTerrain(const Obstacle& o) {
    return o.z - o.depth * 0.5f >= HeightField::farZ
        && o.z + o.depth * 0.5f <= HeightField::nearZ;
}
bool Game::collides(float playerX, float playerFeetY, const GroundHeight& height) const {
    const float halfWidth = PlayerWidth * 0.5f, halfDepth = PlayerDepth * 0.5f;
    for (const auto& o : obstacles_) {
        if (!onTerrain(o)) continue;
        if (o.z + o.depth * 0.5f <= -halfDepth || o.z - o.depth * 0.5f >= halfDepth) continue;
        if (o.x + o.width * 0.5f <= playerX - halfWidth || o.x - o.width * 0.5f >= playerX + halfWidth) continue;
        const float base = height(o.x, o.z);
        if (base + o.height <= playerFeetY || base >= playerFeetY + Player::height) continue;
        return true;
    }
    return false;
}
bool Game::checkCollision(float playerX, float playerFeetY, const GroundHeight& height) {
    if (stress_ || state_ != RunState::Running || !collides(playerX, playerFeetY, height)) return false;
    state_ = RunState::GameOver;
    best_ = std::max(best_, distance_);
    return true;
}
const char* stateName(RunState state) {
    switch (state) {
    case RunState::Ready: return "READY";
    case RunState::Running: return "RUNNING";
    case RunState::Paused: return "PAUSED";
    case RunState::GameOver: return "GAME OVER";
    }
    return "UNKNOWN";
}
const char* obstacleTypeName(int type) {
    switch (type) {
    case 0: return "LOW BARRIER";
    case 1: return "CRATE";
    case 2: return "COLUMN";
    default: return "UNKNOWN";
    }
}
}
