#include "FrameClock.hpp"
#include "../gdr/Timing.hpp"
#include "../store/Settings.hpp"

#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>
#include <unordered_map>

namespace sync {
namespace {

std::unordered_map<std::uintptr_t, double> g_rawTicks;

std::uintptr_t keyFor(PlayLayer* playLayer) {
    return reinterpret_cast<std::uintptr_t>(playLayer);
}

double safeTimelineDenom() {
    double const autoScale = gdr::GD_PHYSICS_TPS / settings::getTimelineGameFps();
    double const fineScale = settings::getTimelineFrameScale();
    double const denom = autoScale * fineScale;
    if (!std::isfinite(denom) || denom <= 1e-9) {
        return 1.0;
    }
    return denom;
}

std::optional<double> gdFrameFromTimelinePosition(PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_player1) {
        return std::nullopt;
    }

    // Unlike X / level-length percent, GD's own timeForPos walks the level
    // timeline/speed changes and gives a time for the current player position.
    float const seconds = playLayer->timeForPos(playLayer->m_player1->getPosition(), 0, 0, true, 0);
    double const gdFrame = static_cast<double>(seconds) * gdr::GD_PHYSICS_TPS;
    if (!std::isfinite(gdFrame) || gdFrame < -5.0 || gdFrame > 3600.0 * gdr::GD_PHYSICS_TPS) {
        return std::nullopt;
    }

    return std::max(0.0, gdFrame);
}

bool levelTimeLooksResetAtNonzeroProgress(PlayLayer* playLayer, double levelTimeSec) {
    if (!playLayer || levelTimeSec > 0.25) {
        return false;
    }

    double const apiPercent = static_cast<double>(playLayer->getCurrentPercent());
    if (std::isfinite(apiPercent) && apiPercent > 0.75) {
        return true;
    }

    if (!playLayer->m_player1) {
        return false;
    }

    double const levelLength = static_cast<double>(playLayer->m_levelLength);
    double const playerX = static_cast<double>(playLayer->m_player1->getPositionX());
    if (!std::isfinite(levelLength) || !std::isfinite(playerX) || levelLength <= 1000.0) {
        return false;
    }

    return playerX / levelLength > 0.01;
}

bool levelTimeLooksRelativeToStartPos(double levelTimeSec, std::optional<double> positionGdFrame) {
    if (!positionGdFrame.has_value()) {
        return false;
    }

    double const positionSec = *positionGdFrame / gdr::GD_PHYSICS_TPS;
    if (!std::isfinite(positionSec) || positionSec <= 1.0) {
        return false;
    }

    // StartPos may make m_gameState.m_levelTime count from the spawn while the
    // replay uses full-level time. If position time stays far ahead, prefer
    // the absolute timeline position instead of constantly snapping to 0..N.
    return positionSec - levelTimeSec > 0.75;
}

} // namespace

void resetFrameClock(PlayLayer* playLayer) {
    if (!playLayer) return;
    g_rawTicks[keyFor(playLayer)] = 0.0;
}

void setFrameClock(PlayLayer* playLayer, double rawTick) {
    if (!playLayer) return;
    g_rawTicks[keyFor(playLayer)] = std::max(0.0, rawTick);
}

void setFrameClockToBaseGdFrame(PlayLayer* playLayer, double gdFrame) {
    setFrameClock(playLayer, rawTickForBaseGdFrame(gdFrame));
}

void advanceFrameClock(PlayLayer* playLayer) {
    if (!playLayer) return;
    g_rawTicks[keyFor(playLayer)] += 1.0;
}

void removeFrameClock(PlayLayer* playLayer) {
    if (!playLayer) return;
    g_rawTicks.erase(keyFor(playLayer));
}

int currentFrame(PlayLayer* playLayer) {
    return static_cast<int>(std::llround(currentFrameExact(playLayer)));
}

double currentFrameExact(PlayLayer* playLayer) {
    if (!playLayer) return 0;
    auto it = g_rawTicks.find(keyFor(playLayer));
    if (it == g_rawTicks.end()) return 0;
    return it->second;
}

std::optional<double> currentLevelTimeGdFrame(PlayLayer* playLayer) {
    if (!playLayer) {
        return std::nullopt;
    }

    auto const positionFrame = gdFrameFromTimelinePosition(playLayer);

    // GD restores this timer on normal play and most practice checkpoint loads.
    // The replay click data is displayed in GD's 240 Hz physics-frame domain.
    double const levelTimeSec = static_cast<double>(playLayer->m_gameState.m_levelTime);
    bool const levelTimeOk = std::isfinite(levelTimeSec) && levelTimeSec >= -0.001 && levelTimeSec <= 3600.0;
    if (levelTimeOk &&
        !levelTimeLooksResetAtNonzeroProgress(playLayer, levelTimeSec) &&
        !levelTimeLooksRelativeToStartPos(levelTimeSec, positionFrame)) {
        return std::max(0.0, levelTimeSec * gdr::GD_PHYSICS_TPS);
    }

    // If StartPos makes m_levelTime relative to the spawn, keep using GD's
    // position-to-time calculation so the overlay remains in full-level replay
    // time instead of jumping back near frame 0.
    if (positionFrame.has_value()) {
        return positionFrame;
    }

    return std::nullopt;
}

double baseGdFrameFromRawTick(double rawTick) {
    if (!std::isfinite(rawTick)) {
        rawTick = 0.0;
    }
    return std::max(0.0, rawTick) * safeTimelineDenom();
}

double displayedGdFrameFromRawTick(double rawTick) {
    return std::round(baseGdFrameFromRawTick(rawTick)) +
        static_cast<double>(settings::getTimelineFrameOffset());
}

double rawTickForBaseGdFrame(double gdFrame) {
    if (!std::isfinite(gdFrame)) {
        gdFrame = 0.0;
    }
    return std::max(0.0, gdFrame) / safeTimelineDenom();
}

bool syncFrameClockToLevelTime(PlayLayer* playLayer) {
    auto const levelTimeFrame = currentLevelTimeGdFrame(playLayer);
    if (!levelTimeFrame.has_value()) {
        return false;
    }
    setFrameClockToBaseGdFrame(playLayer, *levelTimeFrame);
    return true;
}

} // namespace sync
