#pragma once

#include <optional>

namespace settings {

struct SectionBounds {
    double percentFrom = 0.0;
    double percentTo = 5.0;
};

SectionBounds getSectionBounds();

/** nullopt = auto (dominant player) */
std::optional<bool> getPlayer2Filter();

/** User override for full level length in seconds (0 = use replay file). */
std::optional<double> getLevelLengthSecOverride();

/** Visible span in seconds on the pattern overlay (scrolls with gameplay). */
float getTimelineWindowSec();

/** Manual correction applied to frame-counter sync after scaling. */
int getTimelineFrameOffset();

/** Delay after practice/start/checkpoint spawns before percentage-based resync. */
int getPracticeResyncDelay();

/** Active in-game FPS/TPS used to convert the live frame counter into GD 240-frame space. */
double getTimelineGameFps();

/** Manual correction for frame-counter sync. 1.0 = no scale change. */
double getTimelineFrameScale();

/** Minimum frame difference before a resync correction is applied. */
int getTimelineResyncToleranceFrames();

/** Show frame/debug sync data on the overlay HUD. */
bool getTimelineDebugOverlay();

} // namespace settings
