#include "LoadReplay.hpp"
#include "../ui/ModButtonChip.hpp"

#include <Geode/binding/CCMenuItemSpriteExtra.hpp>
#include <Geode/binding/MenuLayer.hpp>
#include <Geode/modify/MenuLayer.hpp>

using namespace geode::prelude;

class $modify(GDRMenuLayer, MenuLayer) {
    bool init() {
        if (!MenuLayer::init()) return false;

        auto* chip = ui::makeModChip("R", 40.f, 32.f);
        auto* btn = CCMenuItemSpriteExtra::create(
            chip,
            this,
            menu_selector(GDRMenuLayer::onGDRPattern)
        );

        auto menu = this->getChildByID("bottom-menu");
        if (menu) {
            menu->addChild(btn);
            btn->setID("gdr-pattern-btn"_spr);
            menu->updateLayout();
        }

        return true;
    }

    void onGDRPattern(CCObject*) {
        hooks::openReplayFilePicker();
    }
};
