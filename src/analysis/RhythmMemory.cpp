#include "RhythmMemory.hpp"
#include "MathUtil.hpp"

#include <sstream>

namespace analysis {

namespace {

std::string speakForBurstSize(int n) {
    switch (n) {
        case 1: return "tap";
        case 2: return "double";
        case 3: return "triplet";
        case 4: return "quad";
        case 5: return "quint";
        default: return std::to_string(n) + "-tap";
    }
}

std::string digitsForBurst(int size) {
    std::string s;
    for (int i = 1; i <= size; ++i) s += std::to_string(i);
    return s;
}

bool hasTrait(Click const& c, Trait t) {
    for (auto const& tr : c.traits)
        if (tr == t) return true;
    return false;
}

} // namespace

RhythmMemory buildRhythmMemory(std::vector<Click> const& clicks) {
    RhythmMemory mem;
    if (clicks.empty()) {
        mem.countLine = mem.speakLine = mem.compactLine = "—";
        return mem;
    }

    std::vector<int> pressGaps;
    for (size_t i = 1; i < clicks.size(); ++i) pressGaps.push_back(clicks[i].pressGapFrames);

    double gapMed = median(pressGaps);
    if (gapMed <= 0) gapMed = 1;

    int tightGap = static_cast<int>(gapMed);
    for (int g : pressGaps) {
        if (g > 0) tightGap = std::min(tightGap, g);
    }
    if (tightGap <= 0) tightGap = static_cast<int>(gapMed);

    double pauseThreshold = std::max({gapMed * 1.45, tightGap * 2.2 + 2.0, gapMed + 3.0});

    std::vector<int> current;
    for (size_t i = 0; i < clicks.size(); ++i) {
        Click const& click = clicks[i];
        Click const* prev = i > 0 ? &clicks[i - 1] : nullptr;

        bool longHoldBreak = prev != nullptr &&
            (hasTrait(*prev, Trait::LongHold) ||
             prev->holdFrames >= static_cast<int>(pauseThreshold * 0.85));

        if (!current.empty() &&
            (click.pressGapFrames >= static_cast<int>(pauseThreshold) || longHoldBreak)) {
            int size = static_cast<int>(current.size());
            mem.bursts.push_back({current, digitsForBurst(size), speakForBurstSize(size)});
            current.clear();
        }
        current.push_back(click.number);
    }

    if (!current.empty()) {
        int size = static_cast<int>(current.size());
        mem.bursts.push_back({current, digitsForBurst(size), speakForBurstSize(size)});
    }

    for (size_t bi = 0; bi < mem.bursts.size(); ++bi) {
        for (int n : mem.bursts[bi].clicks) mem.clickToBurst[n] = static_cast<int>(bi);
    }

    auto join = [](auto const& parts, char const* sep) {
        std::ostringstream os;
        for (size_t i = 0; i < parts.size(); ++i) {
            if (i) os << sep;
            os << parts[i];
        }
        return os.str();
    };

    std::vector<std::string> digits;
    std::vector<std::string> speaks;
    for (auto const& b : mem.bursts) {
        digits.push_back(b.digits);
        speaks.push_back(b.speak);
    }

    mem.countLine = join(digits, " · ");
    mem.speakLine = join(speaks, " — ");
    mem.compactLine = join(digits, "-");

    return mem;
}

std::string buildTraitMnemonic(std::vector<Click> const& clicks) {
    std::ostringstream os;
    for (size_t i = 0; i < clicks.size(); ++i) {
        if (i) os << "·";
        auto const& c = clicks[i];
        if (hasTrait(c, Trait::Late)) os << c.number << "'";
        else if (hasTrait(c, Trait::LongHold)) os << c.number << "_";
        else os << c.number;
    }
    return os.str();
}

} // namespace analysis
