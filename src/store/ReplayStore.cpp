#include "ReplayStore.hpp"
#include "../gdr/ReplayBounds.hpp"
#include "../gdr/Timing.hpp"

#include <algorithm>
#include <cmath>

namespace store {
namespace {

int replayProgressEndFrame(gdr::Replay const& replay) {
    int durationFrames = 0;
    double const fileFps = gdr::replayFileFramerate(replay);
    if (replay.duration > 0.f && fileFps > 0.0) {
        durationFrames = static_cast<int>(std::ceil(static_cast<double>(replay.duration) * fileFps));
    }

    return std::max({gdr::lastDataFrame(replay), durationFrames, 1});
}

std::vector<ReplayProgressPoint> buildProgressLookup(gdr::Replay const& replay, int endFrame) {
    std::vector<ReplayProgressPoint> points;
    int const end = std::max(1, endFrame);
    points.reserve(replay.inputs.size() + replay.deaths.size() + 402);

    auto addFrame = [&](int frame) {
        int const clampedFrame = std::clamp(frame, 0, end);
        double const progressRatio = std::clamp(
            static_cast<double>(clampedFrame) / static_cast<double>(end),
            0.0,
            1.0
        );
        points.push_back({progressRatio, clampedFrame});
    };

    // Dense progress samples make position-only spawns reliable even where the replay has few clicks.
    constexpr int kProgressSamples = 1000; // 0.1% spacing
    for (int i = 0; i <= kProgressSamples; ++i) {
        addFrame(static_cast<int>(std::llround(static_cast<double>(end) * i / kProgressSamples)));
    }

    for (auto const& input : replay.inputs) {
        addFrame(input.frame);
    }
    for (int death : replay.deaths) {
        addFrame(death);
    }

    std::stable_sort(points.begin(), points.end(), [](auto const& a, auto const& b) {
        if (std::abs(a.progressRatio - b.progressRatio) > 1e-9) {
            return a.progressRatio < b.progressRatio;
        }
        return a.replayFrame < b.replayFrame;
    });
    points.erase(std::unique(points.begin(), points.end(), [](auto const& a, auto const& b) {
        return a.replayFrame == b.replayFrame;
    }), points.end());

    return points;
}

} // namespace

ReplayStore& ReplayStore::get() {
    static ReplayStore instance;
    return instance;
}

bool ReplayStore::hasReplay() const {
    std::lock_guard lock(m_mutex);
    return m_replay.has_value();
}

gdr::Replay const& ReplayStore::replay() const {
    std::lock_guard lock(m_mutex);
    return *m_replay;
}

std::vector<ReplayProgressPoint> const& ReplayStore::progressLookup() const {
    std::lock_guard lock(m_mutex);
    return m_progressLookup;
}

int ReplayStore::replayEndFrame() const {
    std::lock_guard lock(m_mutex);
    return m_replayEndFrame;
}

std::string const& ReplayStore::fileName() const {
    std::lock_guard lock(m_mutex);
    return m_fileName;
}

void ReplayStore::setReplay(gdr::Replay replay, std::string fileName) {
    int const endFrame = replayProgressEndFrame(replay);
    auto lookup = buildProgressLookup(replay, endFrame);

    std::lock_guard lock(m_mutex);
    m_replay = std::move(replay);
    m_progressLookup = std::move(lookup);
    m_replayEndFrame = endFrame;
    m_fileName = std::move(fileName);
}

void ReplayStore::clear() {
    std::lock_guard lock(m_mutex);
    m_replay.reset();
    m_progressLookup.clear();
    m_replayEndFrame = 1;
    m_fileName.clear();
}

void ReplayStore::setLastPath(std::string path) {
    std::lock_guard lock(m_mutex);
    m_lastPath = std::move(path);
}

std::string ReplayStore::lastPath() const {
    std::lock_guard lock(m_mutex);
    return m_lastPath;
}

} // namespace store
