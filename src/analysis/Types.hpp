#pragma once

#include <string>
#include <vector>

namespace analysis {

enum class Trait { Late, Early, LongHold, ShortHold };

struct Click {
    int number = 0;
    int pressFrame = 0;
    int releaseFrame = 0;
    int holdFrames = 0;
    int gapFrames = 0;
    int pressGapFrames = 0;
    int relativePress = 0;
    std::vector<Trait> traits;
};

struct RhythmBurst {
    std::vector<int> clicks;
    std::string digits;
    std::string speak;
};

struct Pattern {
    std::string id;
    std::string label;
    std::string description;
    std::string mnemonic;
    std::string traitMnemonic;
    std::string rhythmSpeak;
    std::vector<RhythmBurst> rhythmBursts;
    std::vector<Click> clicks;
    int sectionFrom = 0;
    int sectionTo = 0;
};

struct SectionAnalysis {
    std::vector<Pattern> patterns;
    int totalClicks = 0;
    double framerate = 240.0;
};

} // namespace analysis
