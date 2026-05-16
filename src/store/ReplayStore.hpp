#pragma once

#include "../gdr/Types.hpp"
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace store {

struct ReplayProgressPoint {
    double progressRatio = 0.0;
    int replayFrame = 0;
};

class ReplayStore {
public:
    static ReplayStore& get();

    bool hasReplay() const;
    gdr::Replay const& replay() const;
    std::vector<ReplayProgressPoint> const& progressLookup() const;
    int replayEndFrame() const;
    std::string const& fileName() const;

    void setReplay(gdr::Replay replay, std::string fileName);
    void clear();

    /** Persist path in mod save (optional). */
    void setLastPath(std::string path);
    std::string lastPath() const;

private:
    ReplayStore() = default;

    mutable std::mutex m_mutex;
    std::optional<gdr::Replay> m_replay;
    std::vector<ReplayProgressPoint> m_progressLookup;
    int m_replayEndFrame = 1;
    std::string m_fileName;
    std::string m_lastPath;
};

} // namespace store
