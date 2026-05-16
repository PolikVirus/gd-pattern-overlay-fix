#include "Timing.hpp"

#include <cmath>

namespace gdr {

double clickTimingTps(Replay const& replay) {
    double const file = replay.framerate;
    if (!std::isfinite(file) || file <= 0.0) return GD_PHYSICS_TPS;
    if (std::abs(file - GD_PHYSICS_TPS) <= 2.0) return GD_PHYSICS_TPS;
    if (file < 200.0 || file > 280.0) return GD_PHYSICS_TPS;
    return file;
}

double replayFileFramerate(Replay const& replay) {
    double const file = replay.framerate;
    return std::isfinite(file) && file > 0.0 ? file : GD_PHYSICS_TPS;
}

bool timingTpsDiffersFromFile(Replay const& replay) {
    return std::abs(replayFileFramerate(replay) - clickTimingTps(replay)) > 2.0;
}

} // namespace gdr
