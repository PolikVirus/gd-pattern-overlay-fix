#pragma once

#include "Types.hpp"
#include <unordered_map>

namespace analysis {

struct RhythmMemory {
    std::vector<RhythmBurst> bursts;
    std::string countLine;
    std::string speakLine;
    std::string compactLine;
    std::unordered_map<int, int> clickToBurst;
};

RhythmMemory buildRhythmMemory(std::vector<Click> const& clicks);
std::string buildTraitMnemonic(std::vector<Click> const& clicks);

} // namespace analysis
