#pragma once

#include "Types.hpp"
#include <optional>
#include <vector>

namespace gdr {

int lastDataFrame(Replay const& replay);

double levelLengthSec(Replay const& replay, std::optional<double> overrideSec);

int levelProgressEndFrame(Replay const& replay, std::optional<double> levelLengthSecOverride);

double calibrateLevelLengthSec(int frame, double percent, double tps);

} // namespace gdr
