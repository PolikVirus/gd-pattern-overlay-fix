#include "PatternOverlay.hpp"
#include "../analysis/AnalyzeSection.hpp"
#include "../gdr/ReplayBounds.hpp"
#include "../gdr/Timing.hpp"
#include "../store/ReplayStore.hpp"
#include "../store/Settings.hpp"
#include "../sync/FrameClock.hpp"
#include "../util/LevelProgress.hpp"

#include <Geode/binding/UILayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>

#include <algorithm>
#include <cmath>
#include <optional>
#include <sstream>
#include <vector>

using namespace geode::prelude;

namespace {

char constexpr kUdPx[] = "oasenhase.gdr_pattern_reader.overlay_px";
char constexpr kUdPy[] = "oasenhase.gdr_pattern_reader.overlay_py";

int constexpr kBarPool = 1024;

/** Raw replay input frame index → same instant in GD 240 Hz physics time. */
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

int toGdFrame(int replayFrame, double scale) {
    if (replayFrame <= 0) {
        return 0;
    }
    return static_cast<int>(std::llround(static_cast<double>(replayFrame) * scale));
}

int playLayerEndFrameGuess(PlayLayer* pl) {
    if (!pl) return 1;
    float len = pl->m_levelLength;
    if (len > 0.f) {
        return std::max(1, static_cast<int>(std::round(len * static_cast<float>(gdr::GD_PHYSICS_TPS))));
    }
    return 1;
}

/** Level extent in GD-frame space (matches getCurrentPercent ~ linear in this domain). */
int mapEndGdFrameForSync(PlayLayer* pl, gdr::Replay const& replay, double scale) {
    int const lastGd = toGdFrame(gdr::lastDataFrame(replay), scale);
    int durGd = 0;
    if (replay.duration > 0.0) {
        durGd = static_cast<int>(std::ceil(replay.duration * gdr::GD_PHYSICS_TPS));
    }
    int replayExtentGd = std::max({lastGd, durGd, 1});

    int const playGuess = playLayerEndFrameGuess(pl);

    if (replayExtentGd <= 1 && gdr::lastDataFrame(replay) < 1) {
        return std::max(1, playGuess);
    }
    if (playGuess <= 1) {
        return replayExtentGd;
    }

    double const ratio = static_cast<double>(playGuess) / static_cast<double>(replayExtentGd);
    constexpr double kMinRatio = 0.62;
    constexpr double kMaxRatio = 1.38;
    if (ratio >= kMinRatio && ratio <= kMaxRatio) {
        return std::max(replayExtentGd, playGuess);
    }
    return replayExtentGd;
}

struct ClickGd {
    int pressGd = 0;
    int releaseGd = 0;
};

bool isLongHoldGd(ClickGd const& c) {
    return (c.releaseGd - c.pressGd) >= 8;
}

int trackMaxGd(int mapEndGd, std::vector<ClickGd> const& clicks) {
    int hi = mapEndGd;
    for (auto const& cl : clicks) {
        hi = std::max(hi, cl.releaseGd);
    }
    return std::max(1, hi);
}

int nextClickFrame(std::vector<ClickGd> const& clicks, int currentFrame) {
    auto it = std::lower_bound(clicks.begin(), clicks.end(), currentFrame, [](ClickGd const& click, int frame) {
        return click.pressGd < frame;
    });
    if (it == clicks.end()) {
        return -1;
    }
    return it->pressGd;
}

bool g_overlayMoveMode = false;

class PatternOverlayLayer : public CCLayer {
public:
    static PatternOverlayLayer* create(PlayLayer* pl) {
        auto* r = new PatternOverlayLayer();
        if (r->init(pl)) {
            r->autorelease();
            return r;
        }
        delete r;
        return nullptr;
    }

    void setMoveMode(bool enabled) {
        if (!enabled && (m_moveMode || m_dragging)) {
            savePosition();
        }
        m_moveMode = enabled;
        m_dragging = false;
    }

    bool beginMove(CCTouch* touch) {
        if (!m_moveMode || !touch) {
            return false;
        }
        auto pt = convertTouchToNodeSpace(touch);
        auto const sz = getContentSize();
        if (pt.x < -sz.width / 2.f || pt.x > sz.width / 2.f || pt.y < 0.f || pt.y > sz.height) {
            return false;
        }
        m_dragging = true;
        m_dragTouchStart = touch->getLocation();
        m_dragPosStart = getPosition();
        return true;
    }

    void move(CCTouch* touch) {
        if (!m_dragging || !touch) {
            return;
        }
        auto const loc = touch->getLocation();
        auto const delta = loc - m_dragTouchStart;
        setPosition(m_dragPosStart + delta);
    }

    void endMove() {
        if (m_dragging) {
            savePosition();
        }
        m_dragging = false;
    }

protected:
    bool init(PlayLayer* pl) {
        if (!CCLayer::init() || !pl) {
            return false;
        }
        if (!store::ReplayStore::get().hasReplay()) {
            return false;
        }

        m_playLayer = pl;
        auto const& replay = store::ReplayStore::get().replay();
        m_replayToGd = replayFrameToGdScale(replay);
        m_mapEndGd = mapEndGdFrameForSync(pl, replay, m_replayToGd);

        auto playerFilter = settings::getPlayer2Filter();
        if (!playerFilter.has_value()) {
            playerFilter = analysis::dominantJumpPlayer(replay.inputs);
        }

        int const lastReplay = std::max(1, gdr::lastDataFrame(replay));
        auto rawClicks = analysis::extractClicks(replay.inputs, 0, lastReplay, playerFilter);
        m_clicksGd.reserve(rawClicks.size());
        for (auto const& c : rawClicks) {
            ClickGd g;
            g.pressGd = toGdFrame(c.pressFrame, m_replayToGd);
            g.releaseGd = toGdFrame(c.releaseFrame, m_replayToGd);
            if (g.releaseGd <= g.pressGd) {
                g.releaseGd = g.pressGd + 1;
            }
            m_clicksGd.push_back(g);
        }

        m_bottomPad = 5.f;
        m_trackH = 26.f;
        m_headerH = 20.f;
        float const totalH = m_bottomPad + m_trackH + m_headerH;
        float const totalW = m_tw + 16.f;

        m_baselineY = m_bottomPad + m_trackH * 0.18f;
        m_barBottomY = m_bottomPad + m_trackH * 0.26f;
        m_barH = m_trackH * 0.52f;
        m_tickY = m_bottomPad + m_trackH - 2.f;

        setAnchorPoint({0.5f, 0.f});
        setContentSize({totalW, totalH});

        auto* bg = CCLayerColor::create({10, 14, 28, 200}, static_cast<int>(totalW),
                                        static_cast<int>(totalH));
        bg->setAnchorPoint({0, 0});
        bg->setPosition({-totalW / 2.f, 0});
        addChild(bg);

        float const x0 = -m_tw / 2.f;
        m_baselineStrip = CCLayerColor::create({70, 90, 130, 240}, static_cast<int>(m_tw), 2);
        m_baselineStrip->setAnchorPoint({0, 0});
        m_baselineStrip->setPosition({x0, m_baselineY});
        addChild(m_baselineStrip);

        m_poolGhost.reserve(kBarPool);
        m_poolMain.reserve(kBarPool);
        for (int i = 0; i < kBarPool; ++i) {
            auto* g = CCLayerColor::create({40, 60, 120, 120}, 1, 1);
            g->setVisible(false);
            g->setAnchorPoint({0, 0});
            addChild(g, 4);
            m_poolGhost.push_back(g);

            auto* m = CCLayerColor::create({200, 220, 255, 255}, 1, 1);
            m->setVisible(false);
            m->setAnchorPoint({0, 0});
            addChild(m, 5);
            m_poolMain.push_back(m);
        }

        m_tickLabs.resize(5);
        for (int i = 0; i < 5; ++i) {
            auto* lab = CCLabelBMFont::create("-", "chatFont.fnt");
            if (lab) {
                lab->setScale(0.36f);
                lab->setAnchorPoint({0.5f, 0.f});
                addChild(lab, 30);
            }
            m_tickLabs[i] = lab;
        }

        m_hudLabel = CCLabelBMFont::create("Replay timeline", "chatFont.fnt");
        if (m_hudLabel) {
            m_hudLabel->setScale(0.4f);
            m_hudLabel->setAnchorPoint({0.5f, 0.5f});
            m_hudLabel->setPosition({0, m_bottomPad + m_trackH + m_headerH / 2.f});
            addChild(m_hudLabel, 35);
        }

        m_playhead = CCLayerColor::create({255, 255, 255, 245}, 2,
                                          static_cast<int>(m_trackH + 10));
        m_playhead->setAnchorPoint({0.5f, 0.f});
        m_playhead->setPosition({x0, m_baselineY - 1.f});
        addChild(m_playhead, 100);

        setID("gdr-pattern-overlay"_spr);

        auto const ws = CCDirector::sharedDirector()->getWinSize();
        float const dpx = cocos2d::CCUserDefault::sharedUserDefault()->getFloatForKey(kUdPx, 0.5f);
        float const dpy = cocos2d::CCUserDefault::sharedUserDefault()->getFloatForKey(kUdPy, 0.18f);
        setPosition({dpx * ws.width, dpy * ws.height});

        refreshTimeline();
        scheduleUpdate();
        return true;
    }

    void onEnter() override {
        CCLayer::onEnter();
        setTouchEnabled(true);
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        return beginMove(touch);
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        move(touch);
    }

    void ccTouchEnded(CCTouch* touch, CCEvent*) override {
        (void)touch;
        endMove();
    }

    void ccTouchCancelled(CCTouch* touch, CCEvent*) override {
        (void)touch;
        endMove();
    }

    void update(float /*dt*/) override {
        if (!m_playhead) {
            return;
        }
        auto* pl = PlayLayer::get();
        if (pl != m_playLayer) {
            removeFromParentAndCleanup(true);
            return;
        }
        refreshTimeline();
    }

private:
    void savePosition() {
        auto const ws = CCDirector::sharedDirector()->getWinSize();
        cocos2d::CCUserDefault::sharedUserDefault()->setFloatForKey(kUdPx, getPositionX() / ws.width);
        cocos2d::CCUserDefault::sharedUserDefault()->setFloatForKey(kUdPy, getPositionY() / ws.height);
        cocos2d::CCUserDefault::sharedUserDefault()->flush();
    }

    void refreshTimeline() {
        auto const& replay = store::ReplayStore::get().replay();
        int const trackMax = trackMaxGd(m_mapEndGd, m_clicksGd);

        double const liveFrame = sync::currentFrameExact(m_playLayer);
        double const baseGdFrame = sync::baseGdFrameFromRawTick(liveFrame);
        double const gd = sync::displayedGdFrameFromRawTick(liveFrame);
        auto const levelTimeFrame = sync::currentLevelTimeGdFrame(m_playLayer);
        double const frameFineScale = settings::getTimelineFrameScale();

        int currentFrame = static_cast<int>(std::llround(gd));
        currentFrame = std::clamp(currentFrame, 0, trackMax);

        float const windowSec = settings::getTimelineWindowSec();
        int windowFrames = static_cast<int>(
            std::lround(static_cast<double>(windowSec) * gdr::GD_PHYSICS_TPS));
        windowFrames = std::max(1, windowFrames);
        windowFrames = std::min(windowFrames, trackMax);

        int winStart = currentFrame - windowFrames / 2;
        winStart = std::clamp(winStart, 0, std::max(0, trackMax - windowFrames));
        int const winEnd = std::min(trackMax, winStart + windowFrames);
        double const span = static_cast<double>(std::max(1, winEnd - winStart));

        float const xLeft = -m_tw / 2.f;

        double phFrac =
            (static_cast<double>(currentFrame) - static_cast<double>(winStart)) / span;
        phFrac = std::clamp(phFrac, 0.0, 1.0);
        m_playhead->setPositionX(xLeft + static_cast<float>(phFrac * m_tw));

        for (int i = 0; i < 5; ++i) {
            if (!m_tickLabs[i]) continue;
            double const t = i / 4.0;
            int tf = static_cast<int>(std::lround(static_cast<double>(winStart) + t * span));
            tf = std::clamp(tf, 0, trackMax);
            std::ostringstream os;
            os.setf(std::ios::fixed);
            os.precision(0);
            os << tf << 'f';
            m_tickLabs[i]->setString(os.str().c_str());
            m_tickLabs[i]->setPosition({xLeft + static_cast<float>(t * m_tw), m_tickY});
        }

        if (m_hudLabel) {
            std::ostringstream hud;
            hud.setf(std::ios::fixed);
            hud.precision(1);
            bool const debug = settings::getTimelineDebugOverlay();
            hud << "F " << static_cast<int>(std::llround(baseGdFrame)) << "→" << currentFrame
                << " · " << windowSec << "s";
            int const offset = settings::getTimelineFrameOffset();
            if (debug) {
                hud << " · " << std::clamp(m_playLayer->getCurrentPercent(), 0.f, 100.f) << "%";
                if (levelTimeFrame.has_value()) {
                    hud << " · GD-time " << static_cast<int>(std::llround(*levelTimeFrame)) << 'f';
                }
                hud << " · replay " << currentFrame << 'f';
            } else if (levelTimeFrame.has_value()) {
                hud << " · GD-time→240";
            } else {
                hud << " · " << static_cast<int>(std::lround(settings::getTimelineGameFps()))
                    << "fps→240";
            }
            if ((!levelTimeFrame.has_value() && std::abs(frameFineScale - 1.0) > 0.0005) || offset != 0) {
                hud << " · x" << frameFineScale << " +" << offset << "f";
            }
            if (m_replayToGd < 0.999 || m_replayToGd > 1.001) {
                hud << " · " << static_cast<int>(std::lround(gdr::replayFileFramerate(replay)))
                    << "Hz→240";
            }
            if (m_moveMode) {
                hud << " · MOVE";
            }
            m_hudLabel->setString(hud.str().c_str());
        }

        size_t pi = 0;
        for (auto const& c : m_clicksGd) {
            if (c.releaseGd < winStart || c.pressGd > winEnd) {
                continue;
            }
            if (pi >= m_poolMain.size()) {
                break;
            }

            float x0 = xLeft + (static_cast<float>(c.pressGd - winStart) / span) * m_tw;
            float x1 = xLeft + (static_cast<float>(c.releaseGd - winStart) / span) * m_tw;
            float bw = std::max(2.f, x1 - x0);

            cocos2d::ccColor3B mainRgb =
                isLongHoldGd(c) ? cocos2d::ccColor3B{170, 120, 240} : cocos2d::ccColor3B{175, 210, 255};
            cocos2d::ccColor3B ghostRgb = {static_cast<GLubyte>(mainRgb.r * 0.35f),
                                           static_cast<GLubyte>(mainRgb.g * 0.35f),
                                           static_cast<GLubyte>(mainRgb.b * 0.4f)};

            auto* ghost = m_poolGhost[pi];
            auto* main = m_poolMain[pi];
            ghost->setVisible(true);
            main->setVisible(true);

            int const gw = static_cast<int>(std::ceil(bw)) + 3;
            int const gh = static_cast<int>(m_barH) + 4;
            ghost->setContentSize({static_cast<float>(gw), static_cast<float>(gh)});
            ghost->setColor(ghostRgb);
            ghost->setOpacity(95);
            ghost->setPosition(x0 - 1.5f, m_barBottomY - 2.f);

            int const mw = static_cast<int>(std::ceil(bw));
            int const mh = static_cast<int>(m_barH);
            main->setContentSize({static_cast<float>(mw), static_cast<float>(mh)});
            main->setColor(mainRgb);
            main->setOpacity(235);
            main->setPosition(x0, m_barBottomY);

            ++pi;
        }

        for (; pi < m_poolMain.size(); ++pi) {
            m_poolGhost[pi]->setVisible(false);
            m_poolMain[pi]->setVisible(false);
        }
    }

    PlayLayer* m_playLayer = nullptr;
    std::vector<ClickGd> m_clicksGd;
    int m_mapEndGd = 1;
    double m_replayToGd = 1.0;
    float m_tw = 340.f;
    float m_headerH = 20.f;
    float m_bottomPad = 5.f;
    float m_trackH = 26.f;
    float m_baselineY = 0.f;
    float m_barBottomY = 0.f;
    float m_barH = 0.f;
    float m_tickY = 0.f;

    CCLayerColor* m_baselineStrip = nullptr;
    CCLayerColor* m_playhead = nullptr;
    std::vector<CCLayerColor*> m_poolGhost;
    std::vector<CCLayerColor*> m_poolMain;
    std::vector<CCLabelBMFont*> m_tickLabs;
    CCLabelBMFont* m_hudLabel = nullptr;

    bool m_moveMode = false;
    bool m_dragging = false;
    CCPoint m_dragTouchStart{};
    CCPoint m_dragPosStart{};
};

} // namespace

namespace ui {

void dismissPatternOverlay() {
    g_overlayMoveMode = false;
    auto* pl = PlayLayer::get();
    if (!pl || !pl->m_uiLayer) {
        return;
    }
    if (auto* n = pl->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
        n->removeFromParentAndCleanup(true);
    }
}

void togglePatternOverlay(PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_uiLayer) {
        return;
    }

    if (auto* existing = playLayer->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
        g_overlayMoveMode = false;
        existing->removeFromParentAndCleanup(true);
        return;
    }

    if (!store::ReplayStore::get().hasReplay()) {
        FLAlertLayer::create(
            "Timeline",
            "No replay loaded.\n\nMain menu → tap R (replay) → load a .gdr2 file.",
            "OK"
        )->show();
        return;
    }

    auto* layer = PatternOverlayLayer::create(playLayer);
    if (!layer) {
        return;
    }
    layer->setMoveMode(g_overlayMoveMode);
    playLayer->m_uiLayer->addChild(layer, 2000);
}

void setPatternOverlayMoveMode(PlayLayer* playLayer, bool enabled) {
    g_overlayMoveMode = enabled;
    if (!playLayer || !playLayer->m_uiLayer) {
        return;
    }
    if (auto* n = playLayer->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
        if (auto* overlay = typeinfo_cast<PatternOverlayLayer*>(n)) {
            overlay->setMoveMode(enabled);
        }
    }
}

void togglePatternOverlayMoveMode(PlayLayer* playLayer) {
    setPatternOverlayMoveMode(playLayer, !g_overlayMoveMode);
}

bool isPatternOverlayMoveMode() {
    return g_overlayMoveMode;
}

bool beginPatternOverlayMove(PlayLayer* playLayer, cocos2d::CCTouch* touch) {
    if (!g_overlayMoveMode || !playLayer || !playLayer->m_uiLayer) {
        return false;
    }
    if (auto* n = playLayer->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
        if (auto* overlay = typeinfo_cast<PatternOverlayLayer*>(n)) {
            return overlay->beginMove(touch);
        }
    }
    return false;
}

void movePatternOverlay(PlayLayer* playLayer, cocos2d::CCTouch* touch) {
    if (!playLayer || !playLayer->m_uiLayer) {
        return;
    }
    if (auto* n = playLayer->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
        if (auto* overlay = typeinfo_cast<PatternOverlayLayer*>(n)) {
            overlay->move(touch);
        }
    }
}

void endPatternOverlayMove(PlayLayer* playLayer) {
    if (!playLayer || !playLayer->m_uiLayer) {
        return;
    }
    if (auto* n = playLayer->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
        if (auto* overlay = typeinfo_cast<PatternOverlayLayer*>(n)) {
            overlay->endMove();
        }
    }
}

} // namespace ui
