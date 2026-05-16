#pragma once

#include <optional>

class PlayLayer;

namespace sync {

void resetFrameClock(PlayLayer* playLayer);
void setFrameClock(PlayLayer* playLayer, double rawTick);
void setFrameClockToBaseGdFrame(PlayLayer* playLayer, double gdFrame);
void advanceFrameClock(PlayLayer* playLayer);
void removeFrameClock(PlayLayer* playLayer);
int currentFrame(PlayLayer* playLayer);
double currentFrameExact(PlayLayer* playLayer);

/**
 * Current full-level timeline frame from GD's restored game state, or from
 * PlayLayer::timeForPos when StartPos makes the restored timer spawn-relative.
 * Returned in Geometry Dash's 240 Hz physics-frame space.
 */
std::optional<double> currentLevelTimeGdFrame(PlayLayer* playLayer);

/** Base GD frame before the user's manual overlay offset is applied. */
double baseGdFrameFromRawTick(double rawTick);

/** GD frame shown by the overlay after scaling and manual offset. */
double displayedGdFrameFromRawTick(double rawTick);

/** Raw frame-clock tick that represents the given base GD frame. */
double rawTickForBaseGdFrame(double gdFrame);

/** Snap the frame clock to GD's restored in-level time when available. */
bool syncFrameClockToLevelTime(PlayLayer* playLayer);

} // namespace sync
