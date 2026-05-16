#include "LevelProgress.hpp"
#include "../analysis/Types.hpp"

#include <cmath>
#include <iomanip>
#include <sstream>

namespace util {

double frameToPercent(int frame, int endFrame) {
    if (endFrame <= 0) return 0.0;
    double p = (static_cast<double>(frame) / static_cast<double>(endFrame)) * 100.0;
    return std::min(100.0, std::max(0.0, p));
}

int percentToFrame(double percent, int endFrame) {
    double p = std::min(100.0, std::max(0.0, percent));
    return static_cast<int>(std::round((p / 100.0) * static_cast<double>(endFrame)));
}

std::string formatPercent(double percent, int digits) {
    int d = (percent > 0.0 && percent < 10.0) ? std::max(digits, 2) : digits;
    std::ostringstream os;
    os << std::fixed << std::setprecision(d) << percent << "%";
    return os.str();
}

namespace {
bool hasTrait(analysis::Click const& c, analysis::Trait t) {
    for (auto const& tr : c.traits)
        if (tr == t) return true;
    return false;
}
} // namespace

std::string clickNote(analysis::Click const& click) {
    std::vector<std::string> parts;
    if (hasTrait(click, analysis::Trait::Late)) parts.push_back("Late");
    if (hasTrait(click, analysis::Trait::LongHold)) parts.push_back("Long hold");
    if (hasTrait(click, analysis::Trait::ShortHold)) parts.push_back("Short");
    if (parts.empty()) return "—";
    std::ostringstream os;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i) os << " · ";
        os << parts[i];
    }
    return os.str();
}

} // namespace util
