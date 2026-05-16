#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace gdr {

enum class ReplayFormat { Gdr2, Gdr1, Unknown };

struct Input {
    int frame = 0;
    int button = 1; // 1 = jump
    bool player2 = false;
    bool down = false;
};

struct Replay {
    ReplayFormat format = ReplayFormat::Unknown;
    float duration = 0.f;
    double framerate = 240.0;
    int levelId = 0;
    std::string levelName;
    std::string botName;
    int botVersion = 0;
    bool platformer = false;
    std::vector<Input> inputs;
    std::vector<int> deaths;
};

} // namespace gdr
