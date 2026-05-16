#pragma once

#include "../analysis/Types.hpp"
#include <string>

namespace util {

double frameToPercent(int frame, int endFrame);
int percentToFrame(double percent, int endFrame);
std::string formatPercent(double percent, int digits = 1);

std::string clickNote(analysis::Click const& click);

} // namespace util
