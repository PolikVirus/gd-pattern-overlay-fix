#pragma once

#include "../analysis/Types.hpp"
#include "../gdr/Types.hpp"
#include <string>

namespace ui {

struct PatternViewModel {
    analysis::Pattern const* pattern = nullptr;
    int levelEndFrame = 1;
    std::string bodyText;
};

/** Analyze loaded replay for section bounds. */
PatternViewModel buildPatternView(int playLayerEndFrame);

std::string formatPatternBody(
    analysis::Pattern const& pattern,
    int levelEndFrame,
    double timingTps,
    gdr::Replay const& replay
);

} // namespace ui
