#include "../ui/PatternPopup.hpp"
#include "../ui/ModButtonChip.hpp"
#include "../ui/PatternOverlay.hpp"

#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/PauseLayer.hpp>
#include <Geode/binding/PlayLayer.hpp>
#include <Geode/loader/Mod.hpp>
#include <Geode/modify/PauseLayer.hpp>
#include <Geode/ui/GeodeUI.hpp>

using namespace geode::prelude;

namespace {

char constexpr kMoveCaptureId[] = "gdr-pattern-move-capture";

class OverlayMoveCaptureLayer : public CCLayer {
public:
    static OverlayMoveCaptureLayer* create() {
        auto* ret = new OverlayMoveCaptureLayer();
        if (ret && ret->init()) {
            ret->autorelease();
            return ret;
        }
        delete ret;
        return nullptr;
    }

    bool init() override {
        if (!CCLayer::init()) {
            return false;
        }
        setID(kMoveCaptureId);
        setContentSize(CCDirector::sharedDirector()->getWinSize());
        setTouchMode(cocos2d::kCCTouchesOneByOne);
        setTouchPriority(-10000);
        setTouchEnabled(true);
        return true;
    }

    bool ccTouchBegan(CCTouch* touch, CCEvent*) override {
        m_dragging = ui::beginPatternOverlayMove(PlayLayer::get(), touch);
        return m_dragging;
    }

    void ccTouchMoved(CCTouch* touch, CCEvent*) override {
        if (!m_dragging) {
            return;
        }
        ui::movePatternOverlay(PlayLayer::get(), touch);
    }

    void ccTouchEnded(CCTouch* touch, CCEvent*) override {
        (void)touch;
        if (!m_dragging) {
            return;
        }
        ui::endPatternOverlayMove(PlayLayer::get());
        m_dragging = false;
    }

    void ccTouchCancelled(CCTouch* touch, CCEvent* event) override {
        ccTouchEnded(touch, event);
    }

private:
    bool m_dragging = false;
};

void updateMoveCapture(PauseLayer* layer) {
    if (!layer) {
        return;
    }
    if (auto* existing = layer->getChildByID(kMoveCaptureId)) {
        existing->removeFromParentAndCleanup(true);
    }
    if (!ui::isPatternOverlayMoveMode()) {
        return;
    }
    if (auto* capture = OverlayMoveCaptureLayer::create()) {
        layer->addChild(capture, 10000);
    }
}

}

class $modify(GDRPauseLayer, PauseLayer) {
    void customSetup() {
        PauseLayer::customSetup();

        auto const ws = CCDirector::sharedDirector()->getWinSize();
        auto* menu = CCMenu::create();
        if (!menu) {
            return;
        }
        menu->setID("gdr-pattern-right-menu"_spr);
        menu->setContentSize({64.f, 132.f});
        menu->setPosition({ws.width - 42.f, ws.height * 0.5f});
        addChild(menu, 5000);

        auto* timelineBtn = CCMenuItemSpriteExtra::create(
            ui::makeModChip("TL", 38.f, 30.f),
            this,
            menu_selector(GDRPauseLayer::onPattern)
        );
        timelineBtn->setID("gdr-pattern-pause-btn"_spr);
        timelineBtn->setPosition({0.f, 42.f});
        menu->addChild(timelineBtn);

        auto* moveBtn = CCMenuItemSpriteExtra::create(
            ui::makeModChip("Move", 54.f, 30.f),
            this,
            menu_selector(GDRPauseLayer::onMoveOverlay)
        );
        moveBtn->setID("gdr-pattern-move-btn"_spr);
        moveBtn->setPosition({0.f, 0.f});
        menu->addChild(moveBtn);

        auto* settingsBtn = CCMenuItemSpriteExtra::create(
            ui::makeModChip("Set", 44.f, 30.f),
            this,
            menu_selector(GDRPauseLayer::onPatternSettings)
        );
        settingsBtn->setID("gdr-pattern-settings-btn"_spr);
        settingsBtn->setPosition({0.f, -42.f});
        menu->addChild(settingsBtn);

        updateMoveCapture(this);
    }

    void onPattern(CCObject*) {
        auto* pl = PlayLayer::get();
        ui::showPatternPopup(pl);
    }

    void onMoveOverlay(CCObject*) {
        auto* pl = PlayLayer::get();
        if (pl && !pl->m_uiLayer->getChildByID("gdr-pattern-overlay"_spr)) {
            ui::togglePatternOverlay(pl);
        }
        ui::togglePatternOverlayMoveMode(pl);
        updateMoveCapture(this);
    }

    void onPatternSettings(CCObject*) {
        geode::openSettingsPopup(Mod::get(), true);
    }
};
