#include "AnalyzeSection.hpp"
#include "MathUtil.hpp"
#include "RhythmMemory.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

namespace analysis {

namespace {
constexpr int MAX_CLICKS_PER_PATTERN = 8;
constexpr double LATE_RATIO = 1.35;
constexpr int LONG_HOLD_FRAMES = 8;
constexpr int SHORT_HOLD_FRAMES = 2;

bool hasTrait(Click const& c, Trait t) {
    for (auto const& tr : c.traits)
        if (tr == t) return true;
    return false;
}

void annotateTraits(std::vector<Click>& clicks) {
    if (clicks.empty()) return;

    std::vector<int> gaps;
    for (size_t i = 1; i < clicks.size(); ++i) gaps.push_back(clicks[i].gapFrames);
    double gapMed = median(gaps);

    std::vector<int> holds;
    for (auto const& c : clicks) holds.push_back(c.holdFrames);
    double holdMed = median(holds);

    for (size_t i = 0; i < clicks.size(); ++i) {
        auto& c = clicks[i];
        if (i > 0 && gapMed > 0 && c.gapFrames >= static_cast<int>(gapMed * LATE_RATIO)) {
            c.traits.push_back(Trait::Late);
        }
        if (i > 0 && gapMed > 0 && c.gapFrames <= static_cast<int>(gapMed * 0.55)) {
            c.traits.push_back(Trait::Early);
        }
        if (c.holdFrames >= std::max(LONG_HOLD_FRAMES, static_cast<int>(holdMed * 1.6))) {
            c.traits.push_back(Trait::LongHold);
        }
        if (c.holdFrames <= SHORT_HOLD_FRAMES && c.holdFrames < static_cast<int>(holdMed * 0.6)) {
            c.traits.push_back(Trait::ShortHold);
        }
    }
}

struct Described {
    std::string label;
    std::string description;
    std::string mnemonic;
    std::string traitMnemonic;
    std::string rhythmSpeak;
    std::vector<RhythmBurst> rhythmBursts;
};

Described describePattern(std::vector<Click> const& clicks, int selectionFrom, int selectionTo) {
    int n = static_cast<int>(clicks.size());
    if (n == 0) {
        return {"Empty", "No jump clicks in this section.", "—", "—", "—", {}};
    }

    int lateCount = 0;
    for (auto const& c : clicks)
        if (hasTrait(c, Trait::Late)) ++lateCount;
    bool lastLate = hasTrait(clicks.back(), Trait::Late);

    std::vector<int> pressGaps;
    for (size_t i = 1; i < clicks.size(); ++i) pressGaps.push_back(clicks[i].pressGapFrames);
    int gapMed = static_cast<int>(std::round(median(pressGaps)));

    std::string label = std::to_string(n) + (n == 1 ? " click" : " clicks");
    if (lastLate && n >= 3) label = std::to_string(n) + "-tap · late finish";
    else if (lateCount > 0) label = std::to_string(n) + "-tap · " + std::to_string(lateCount) + " delayed";

    std::ostringstream parts;
    parts << n << (n == 1 ? " jump" : " jumps");

    int selSpan = std::max(1, selectionTo - selectionFrom);
    int leadFrames = clicks[0].pressFrame - selectionFrom;
    if (leadFrames > selSpan * 0.08) {
        parts << " · starts +" << leadFrames << "f into selection";
    }
    if (gapMed > 0 && !pressGaps.empty()) {
        parts << " · ~" << gapMed << "f between taps";
    }
    if (lastLate) {
        double prevMed = pressGaps.size() > 1 ? median(std::vector<int>(pressGaps.begin(), pressGaps.end() - 1))
                                              : gapMed;
        int extra = clicks.back().gapFrames - static_cast<int>(prevMed);
        parts << " · last tap +" << extra << "f late";
    }

    auto rhythm = buildRhythmMemory(clicks);
    if (rhythm.bursts.size() > 1) {
        parts << " · remember: " << rhythm.countLine;
    }

    return {label, parts.str(), rhythm.countLine, buildTraitMnemonic(clicks),
            rhythm.speakLine, rhythm.bursts};
}

std::vector<std::vector<Click>> splitOnLargeGaps(std::vector<Click> const& clicks) {
    if (static_cast<int>(clicks.size()) <= MAX_CLICKS_PER_PATTERN) {
        return {clicks};
    }

    std::vector<int> pressGaps;
    for (size_t i = 1; i < clicks.size(); ++i) pressGaps.push_back(clicks[i].pressGapFrames);
    double gapMed = median(pressGaps);
    if (gapMed <= 0) gapMed = 1;
    double threshold = gapMed * 2.2;

    std::vector<std::vector<Click>> chunks;
    std::vector<Click> current;

    for (size_t i = 0; i < clicks.size(); ++i) {
        Click const& click = clicks[i];
        Click const* prev = i > 0 ? &clicks[i - 1] : nullptr;

        bool longHoldBreak = prev != nullptr &&
            (hasTrait(*prev, Trait::LongHold) ||
             prev->holdFrames >= std::max(LONG_HOLD_FRAMES, static_cast<int>(threshold * 0.7)));

        if (!current.empty() && static_cast<int>(current.size()) >= 3 &&
            (click.pressGapFrames >= static_cast<int>(threshold) || longHoldBreak)) {
            chunks.push_back(current);
            current.clear();
        }
        current.push_back(click);
        if (static_cast<int>(current.size()) >= MAX_CLICKS_PER_PATTERN) {
            chunks.push_back(current);
            current.clear();
        }
    }
    if (!current.empty()) chunks.push_back(current);
    if (chunks.empty()) return {clicks};
    return chunks;
}

Pattern buildPattern(std::vector<Click> clicks, int selectionFrom, int selectionTo, int index,
                     int total) {
    annotateTraits(clicks);
    auto d = describePattern(clicks, selectionFrom, selectionTo);

    int patternFrom = clicks.empty() ? selectionFrom : clicks.front().pressFrame;
    int patternTo = clicks.empty() ? selectionTo : clicks.back().releaseFrame;

    std::string suffix = total > 1 ? " (" + std::to_string(index + 1) + "/" + std::to_string(total) + ")" : "";
    std::ostringstream id;
    id << "pattern-" << patternFrom << "-" << patternTo << "-" << index;

    return {id.str(),
            d.label + suffix,
            d.description,
            d.mnemonic,
            d.traitMnemonic,
            d.rhythmSpeak,
            d.rhythmBursts,
            std::move(clicks),
            patternFrom,
            patternTo};
}

} // namespace

bool dominantJumpPlayer(std::vector<gdr::Input> const& inputs) {
    int p1 = 0, p2 = 0;
    for (auto const& i : inputs) {
        if (i.button != 1 || !i.down) continue;
        if (i.player2) ++p2;
        else ++p1;
    }
    return p2 > p1;
}

std::vector<Click> extractClicks(std::vector<gdr::Input> const& inputs, int from, int to,
                                 std::optional<bool> player2) {
    std::vector<gdr::Input> filtered;
    for (auto const& i : inputs) {
        if (i.button != 1) continue;
        if (i.frame < from || i.frame > to) continue;
        if (player2.has_value() && i.player2 != *player2) continue;
        filtered.push_back(i);
    }
    std::sort(filtered.begin(), filtered.end(), [](auto const& a, auto const& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return static_cast<int>(a.player2) < static_cast<int>(b.player2);
    });

    std::vector<Click> clicks;
    int lastRelease = from;
    int lastPress = from;

    for (auto const& press : filtered) {
        if (!press.down) continue;

        int releaseFrame = press.frame + 3;
        for (auto const& i : filtered) {
            if (!i.down && i.frame >= press.frame && i.player2 == press.player2 &&
                i.button == press.button) {
                releaseFrame = i.frame;
                break;
            }
        }

        Click c;
        c.number = static_cast<int>(clicks.size()) + 1;
        c.pressFrame = press.frame;
        c.releaseFrame = releaseFrame;
        c.holdFrames = std::max(1, releaseFrame - press.frame);
        c.gapFrames = std::max(0, press.frame - lastRelease);
        c.pressGapFrames = std::max(0, press.frame - lastPress);
        c.relativePress = press.frame - from;
        clicks.push_back(c);

        lastRelease = releaseFrame;
        lastPress = press.frame;
    }

    for (size_t i = 0; i < clicks.size(); ++i) clicks[i].number = static_cast<int>(i) + 1;
    return clicks;
}

SectionAnalysis analyzeSection(std::vector<gdr::Input> const& inputs, int from, int to,
                               double framerate, std::optional<bool> player2) {
    int safeFrom = std::min(from, to);
    int safeTo = std::max(from, to);

    auto clicks = extractClicks(inputs, safeFrom, safeTo, player2);
    SectionAnalysis result;
    result.framerate = framerate;
    result.totalClicks = static_cast<int>(clicks.size());

    if (clicks.empty()) {
        result.patterns.push_back(buildPattern({}, safeFrom, safeTo, 0, 1));
        return result;
    }

    auto chunks = splitOnLargeGaps(clicks);
    int idx = 0;
    int total = static_cast<int>(chunks.size());
    for (auto& chunk : chunks) {
        if (chunk.empty()) continue;
        result.patterns.push_back(buildPattern(std::move(chunk), safeFrom, safeTo, idx++, total));
    }
    return result;
}

} // namespace analysis
