#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

namespace analysis {

inline double median(std::vector<int> values) {
    if (values.empty()) return 0.0;
    std::sort(values.begin(), values.end());
    size_t mid = values.size() / 2;
    if (values.size() % 2) return static_cast<double>(values[mid]);
    return (static_cast<double>(values[mid - 1]) + static_cast<double>(values[mid])) / 2.0;
}

} // namespace analysis
