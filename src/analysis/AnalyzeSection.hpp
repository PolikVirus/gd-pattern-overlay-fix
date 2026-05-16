#pragma once

#include "../gdr/Types.hpp"
#include "Types.hpp"
#include <optional>

namespace analysis {

std::vector<Click> extractClicks(
    std::vector<gdr::Input> const& inputs,
    int from,
    int to,
    std::optional<bool> player2
);

SectionAnalysis analyzeSection(
    std::vector<gdr::Input> const& inputs,
    int from,
    int to,
    double framerate,
    std::optional<bool> player2
);

/** Pick P1 vs P2 from jump press counts (matches web dominantJumpPlayer). */
bool dominantJumpPlayer(std::vector<gdr::Input> const& inputs);

} // namespace analysis
