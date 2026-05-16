#include "../ui/PatternOverlay.hpp"
#include "../gdr/Timing.hpp"
#include "../store/ReplayStore.hpp"
#include "../store/Settings.hpp"
#include "../sync/FrameClock.hpp"

#include <Geode/binding/PlayLayer.hpp>
#include <Geode/binding/PlayerObject.hpp>
#include <Geode/modify/PlayLayer.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <type_traits>
#include <utility>

using namespace geode::prelude;

namespace {

double replayFrameToGdScale(gdr::Replay const& replay) {
    double const fileFps = gdr::replayFileFramerate(replay);
    if (!std::isfinite(fileFps) || fileFps < 30.0 || fileFps > 720.0) {
        return 1.0;
    }
    if (std::abs(fileFps - gdr::GD_PHYSICS_TPS) < 0.5) {
        return 1.0;
    }
    return gdr::GD_PHYSICS_TPS / fileFps;
}

double replayFrameForProgressRatio(double progressRatio) {
    auto const& points = store::ReplayStore::get().progressLookup();
    if (points.empty()) {
        int const endFrame = std::max(1, store::ReplayStore::get().replayEndFrame());
        return std::clamp(progressRatio, 0.0, 1.0) * static_cast<double>(endFrame);
    }

    double const ratio = std::clamp(progressRatio, 0.0, 1.0);
    auto it = std::lower_bound(points.begin(), points.end(), ratio, [](auto const& pt, double value) {
        return pt.progressRatio < value;
    });
    if (it == points.begin()) {
        return static_cast<double>(it->replayFrame);
    }
    if (it == points.end()) {
        return static_cast<double>(points.back().replayFrame);
    }
    auto const& prev = *(it - 1);
    return (std::abs(prev.progressRatio - ratio) <= std::abs(it->progressRatio - ratio)) ?
        static_cast<double>(prev.replayFrame) :
        static_cast<double>(it->replayFrame);
}

template <class T, class = void>
struct HasGetPercentage : std::false_type {};

template <class T>
struct HasGetPercentage<T, std::void_t<decltype(std::declval<T*>()->getPercentage())>> : std::true_type {};

template <class T>
double percentFromPlayLayerApi(T* pl) {
    if constexpr (HasGetPercentage<T>::value) {
        return static_cast<double>(pl->getPercentage());
    } else {
        return static_cast<double>(pl->getCurrentPercent());
    }
}

std::optional<double> percentFromPlayerPosition(PlayLayer* pl) {
    if (!pl || !pl->m_player1) {
        return std::nullopt;
    }

    double const levelLength = static_cast<double>(pl->m_levelLength);
    double const playerX = static_cast<double>(pl->m_player1->m_position.x);
    if (!std::isfinite(levelLength) || !std::isfinite(playerX) || levelLength <= 1000.0 || playerX < 0.0) {
        return std::nullopt;
    }

    double const percent = playerX * 100.0 / levelLength;
    if (percent < -0.5 || percent > 100.5) {
        return std::nullopt;
    }
    return std::clamp(percent, 0.0, 100.0);
}

std::optional<double> progressRatioFromPlayerPosition(PlayLayer* pl) {
    if (!pl || !pl->m_player1) {
        return std::nullopt;
    }

    double const levelLength = static_cast<double>(pl->m_levelLength);
    double const playerX = static_cast<double>(pl->m_player1->getPositionX());
    if (!std::isfinite(levelLength) || !std::isfinite(playerX) || levelLength <= 1000.0 || playerX < 0.0) {
        return std::nullopt;
    }

    double const ratio = playerX / levelLength;
    if (ratio < -0.005 || ratio > 1.005) {
        return std::nullopt;
    }
    return std::clamp(ratio, 0.0, 1.0);
}

std::optional<double> gdFrameFromTimelinePosition(PlayLayer* pl) {
    if (!pl || !pl->m_player1) {
        return std::nullopt;
    }

    // Unlike a plain X / level-length percent, GD's timeForPos accounts for
    // speed changes when converting a level position back into song/level time.
    float const seconds = pl->timeForPos(pl->m_player1->getPosition(), 0, 0, true, 0);
    double const gdFrame = static_cast<double>(seconds) * gdr::GD_PHYSICS_TPS;
    if (!std::isfinite(gdFrame) || gdFrame < -5.0 || gdFrame > 3600.0 * gdr::GD_PHYSICS_TPS) {
        return std::nullopt;
    }

    return std::max(0.0, gdFrame);
}

double currentLevelPercent(PlayLayer* pl) {
    if (!pl) {
        return 0.0;
    }
    double const apiPercent = std::clamp(percentFromPlayLayerApi(pl), 0.0, 100.0);
    auto const playerPercent = percentFromPlayerPosition(pl);
    if (!playerPercent.has_value()) {
        return apiPercent;
    }
    if (apiPercent <= 0.05 && *playerPercent > 0.05) {
        return *playerPercent;
    }
    if (std::abs(*playerPercent - apiPercent) <= 5.0) {
        return *playerPercent;
    }
    return apiPercent;
}

double resyncToCurrentPosition(PlayLayer* pl, double toleranceFrames) {
    if (!pl || !store::ReplayStore::get().hasReplay()) {
        sync::resetFrameClock(pl);
        return 0.0;
    }

    auto const& replay = store::ReplayStore::get().replay();

    double targetGdFrame = 0.0;
    if (auto const levelTimeFrame = sync::currentLevelTimeGdFrame(pl)) {
        targetGdFrame = *levelTimeFrame;
    } else if (auto const positionTimeFrame = gdFrameFromTimelinePosition(pl)) {
        targetGdFrame = *positionTimeFrame;
    } else {
        double const progressRatio = progressRatioFromPlayerPosition(pl)
            .value_or(std::clamp(currentLevelPercent(pl) / 100.0, 0.0, 1.0));
        double const replayFrame = replayFrameForProgressRatio(progressRatio);
        targetGdFrame = replayFrame * replayFrameToGdScale(replay);
    }

    double const currentGdFrame = sync::baseGdFrameFromRawTick(sync::currentFrameExact(pl));
    double const diffGd = targetGdFrame - currentGdFrame;
    if (toleranceFrames > 0.0 && std::abs(diffGd) <= toleranceFrames) {
        return diffGd;
    }

    sync::setFrameClockToBaseGdFrame(pl, targetGdFrame);
    return diffGd;
}

} // namespace

class $modify(GDRPatternPlayQuit, PlayLayer) {
    struct Fields {
        int m_resyncDelayFrames = 0;
        double m_lastPercent = -1.0;
    };

    void startGame() {
        PlayLayer::startGame();
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void startGameDelayed() {
        PlayLayer::startGameDelayed();
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void resetLevel() {
        PlayLayer::resetLevel();
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void resetLevelFromStart() {
        PlayLayer::resetLevelFromStart();
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void fullReset() {
        PlayLayer::fullReset();
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void loadFromCheckpoint(CheckpointObject* object) {
        PlayLayer::loadFromCheckpoint(object);
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void togglePracticeMode(bool practiceMode) {
        PlayLayer::togglePracticeMode(practiceMode);
        m_fields->m_resyncDelayFrames = settings::getPracticeResyncDelay();
        m_fields->m_lastPercent = -1.0;
    }

    void postUpdate(float dt) {
        PlayLayer::postUpdate(dt);
        (void)dt;

        if (!sync::syncFrameClockToLevelTime(this)) {
            sync::advanceFrameClock(this);
        }

        if (m_fields->m_resyncDelayFrames > 0) {
            m_fields->m_resyncDelayFrames--;

            double currentPct = currentLevelPercent(this);
            if (m_fields->m_resyncDelayFrames <= 6 ||
                (m_fields->m_lastPercent > 0.0 && std::abs(currentPct - m_fields->m_lastPercent) < 0.2)) {
                resyncToCurrentPosition(this, static_cast<double>(settings::getTimelineResyncToleranceFrames()));
                m_fields->m_resyncDelayFrames = 0;
            }

            m_fields->m_lastPercent = currentPct;
        }
    }

    void onQuit() {
        ui::dismissPatternOverlay();
        sync::removeFrameClock(this);
        PlayLayer::onQuit();
    }
};
