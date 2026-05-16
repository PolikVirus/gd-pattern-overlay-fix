#include "Settings.hpp"
#include <Geode/loader/Mod.hpp>

#include <algorithm>
#include <cmath>

using namespace geode::prelude;

namespace settings {

SectionBounds getSectionBounds() {
    SectionBounds b;
    b.percentFrom = Mod::get()->getSettingValue<float>("percent-from");
    b.percentTo = Mod::get()->getSettingValue<float>("percent-to");
    if (b.percentTo < b.percentFrom) std::swap(b.percentFrom, b.percentTo);
    return b;
}

std::optional<bool> getPlayer2Filter() {
    auto v = Mod::get()->getSettingValue<std::string>("player-filter");
    if (v == "p1") return false;
    if (v == "p2") return true;
    return std::nullopt;
}

std::optional<double> getLevelLengthSecOverride() {
    float v = Mod::get()->getSettingValue<float>("level-length-sec");
    if (v > 0.f) return static_cast<double>(v);
    return std::nullopt;
}

float getTimelineWindowSec() {
    float v = Mod::get()->getSettingValue<float>("timeline-window-sec");
    if (v < 2.f) v = 2.f;
    if (v > 30.f) v = 30.f;
    return v;
}

int getTimelineFrameOffset() {
    float v = Mod::get()->getSettingValue<float>("timeline-frame-offset");
    int k = static_cast<int>(std::lround(v));
    if (k < -100000) return -100000;
    if (k > 100000) return 100000;
    return k;
}

int getPracticeResyncDelay() {
    float v = Mod::get()->getSettingValue<float>("practice-resync-delay-frames");
    int k = static_cast<int>(std::lround(v));
    if (k < 3) return 3;
    if (k > 30) return 30;
    return k;
}

double getTimelineGameFps() {
    float v = Mod::get()->getSettingValue<float>("timeline-game-fps");
    if (v < 30.f) v = 30.f;
    if (v > 1000.f) v = 1000.f;
    return static_cast<double>(v);
}

double getTimelineFrameScale() {
    float v = Mod::get()->getSettingValue<float>("timeline-frame-scale");
    if (v < 0.01f) v = 0.01f;
    if (v > 2.0f) v = 2.0f;
    return static_cast<double>(v);
}

int getTimelineResyncToleranceFrames() {
    float v = Mod::get()->getSettingValue<float>("timeline-resync-tolerance");
    int k = static_cast<int>(std::lround(v));
    if (k < 0) return 0;
    if (k > 120) return 120;
    return k;
}

bool getTimelineDebugOverlay() {
    return Mod::get()->getSettingValue<bool>("timeline-debug-overlay");
}

} // namespace settings
