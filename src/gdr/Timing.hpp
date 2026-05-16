#pragma once

#include "Types.hpp"

namespace gdr {

constexpr double GD_PHYSICS_TPS = 240.0;

/** Ticks per second for level % (matches web app). */
double clickTimingTps(Replay const& replay);

double replayFileFramerate(Replay const& replay);

bool timingTpsDiffersFromFile(Replay const& replay);

} // namespace gdr
