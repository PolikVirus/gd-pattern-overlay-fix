#include "PatternService.hpp"
#include "../analysis/AnalyzeSection.hpp"
#include "../gdr/ReplayBounds.hpp"
#include "../gdr/Timing.hpp"
#include "../store/ReplayStore.hpp"
#include "../store/Settings.hpp"
#include "../util/LevelProgress.hpp"

#include <cmath>
#include <sstream>

namespace ui {

namespace {

int resolveLevelEndFrame(gdr::Replay const& replay, int playLayerEndFrame) {
    auto overrideSec = settings::getLevelLengthSecOverride();
    int fromReplay = gdr::levelProgressEndFrame(replay, overrideSec);
    if (playLayerEndFrame > 1) {
        return std::max(fromReplay, playLayerEndFrame);
    }
    return fromReplay;
}

} // namespace

std::string formatPatternBody(
    analysis::Pattern const& pattern,
    int levelEndFrame,
    double timingTps,
    gdr::Replay const& replay
) {
    std::ostringstream os;
    os << pattern.label << "\n\n";
    os << pattern.description << "\n\n";
    os << "Count: " << pattern.mnemonic << "\n";
    os << "Say: " << pattern.rhythmSpeak << "\n";
    os << "Detail: " << pattern.traitMnemonic << "\n\n";

    double const activeSec = gdr::levelLengthSec(replay, settings::getLevelLengthSecOverride());
    os << "Level length: " << activeSec << "s";
    if (settings::getLevelLengthSecOverride().has_value()) os << " (your setting)";
    os << " · " << timingTps << " TPS";
    if (gdr::timingTpsDiffersFromFile(replay)) {
        os << " (file: " << gdr::replayFileFramerate(replay) << ")";
    }
    os << "\n\n";

    if (pattern.clicks.empty()) {
        os << "(no clicks in section)";
        return os.str();
    }

    os << "#   Level %   Frame   Note\n";
    os << "────────────────────────────────\n";
    for (auto const& c : pattern.clicks) {
        os << c.number << "   "
           << util::formatPercent(util::frameToPercent(c.pressFrame, levelEndFrame), 1)
           << "   f" << c.pressFrame << "   " << util::clickNote(c) << "\n";
    }
    return os.str();
}

PatternViewModel buildPatternView(int playLayerEndFrame) {
    PatternViewModel vm;
    vm.levelEndFrame = std::max(1, playLayerEndFrame);

    if (!store::ReplayStore::get().hasReplay()) {
        vm.bodyText =
            "No replay loaded.\n\n"
            "Main menu → GDR → load a .gdr2 file.\n"
            "(.gdr xdBot: export as .gdr2 or use the web app.)";
        return vm;
    }

    auto const& replay = store::ReplayStore::get().replay();
    double const timingTps = gdr::clickTimingTps(replay);
    vm.levelEndFrame = resolveLevelEndFrame(replay, playLayerEndFrame);

    auto bounds = settings::getSectionBounds();
    int from = util::percentToFrame(bounds.percentFrom, vm.levelEndFrame);
    int to = util::percentToFrame(bounds.percentTo, vm.levelEndFrame);

    auto playerFilter = settings::getPlayer2Filter();
    if (!playerFilter.has_value()) {
        playerFilter = analysis::dominantJumpPlayer(replay.inputs);
    }

    auto analysis = analysis::analyzeSection(replay.inputs, from, to, timingTps, playerFilter);

    if (analysis.patterns.empty() || analysis.patterns.front().clicks.empty()) {
        vm.bodyText = "No jump clicks in this section.\n\n"
                      "Widen Percent from / to in mod settings.";
        return vm;
    }

    static analysis::Pattern s_cached;
    s_cached = analysis.patterns.front();
    vm.pattern = &s_cached;
    vm.bodyText = formatPatternBody(s_cached, vm.levelEndFrame, timingTps, replay);
    return vm;
}

} // namespace ui
