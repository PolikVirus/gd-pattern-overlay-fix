#pragma once

#include <Geode/Geode.hpp>

namespace ui {

/** Small high-contrast menu chip (label on padded background). */
inline cocos2d::CCNode* makeModChip(char const* labelText, float w = 38.f, float h = 30.f) {
    using namespace geode::prelude;
    auto* node = cocos2d::CCNode::create();
    node->setContentSize({w, h});

    auto* pad = cocos2d::CCLayerColor::create(
        cocos2d::ccColor4B{48, 62, 98, 250}, static_cast<int>(w), static_cast<int>(h));
    pad->setAnchorPoint({0.f, 0.f});

    auto* lab = cocos2d::CCLabelBMFont::create(labelText, "bigFont.fnt");
    if (lab) {
        lab->setScale(0.42f);
        lab->setColor({255, 248, 210});
        lab->setPosition({w * 0.5f, h * 0.5f});
    }

    node->addChild(pad);
    if (lab) {
        node->addChild(lab);
    }
    return node;
}

} // namespace ui
