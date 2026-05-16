#include "ReplayBounds.hpp"
#include "Timing.hpp"

#include <algorithm>
#include <cmath>

namespace gdr {

int lastDataFrame(Replay const& replay) {
    int m = 0;
    for (auto const& i : replay.inputs) m = std::max(m, i.frame);
    for (int d : replay.deaths) m = std::max(m, d);
    return m;
}

double levelLengthSec(Replay const& replay, std::optional<double> overrideSec) {
    if (overrideSec.has_value() && *overrideSec > 0.0) return *overrideSec;
    return replay.duration > 0.0 ? replay.duration : 0.0;
}

int levelProgressEndFrame(Replay const& replay, std::optional<double> levelLengthSecOverride) {
    double const tps = clickTimingTps(replay);
    if (levelLengthSecOverride.has_value() && *levelLengthSecOverride > 0.0) {
        return std::max(1, static_cast<int>(std::ceil(*levelLengthSecOverride * tps)));
    }

    int const fromData = lastDataFrame(replay);
    int const fromHeader = static_cast<int>(std::ceil(replay.duration * tps));

    if (fromHeader > static_cast<int>(fromData * 1.05)) {
        return std::max({fromHeader, fromData, 1});
    }

    return std::max({fromData, fromHeader, 1});
}

double calibrateLevelLengthSec(int frame, double percent, double tps) {
    double p = std::min(100.0, std::max(0.01, percent));
    if (frame <= 0 || tps <= 0.0) return 0.0;
    return static_cast<double>(frame) / (p / 100.0) / tps;
}

} // namespace gdr
