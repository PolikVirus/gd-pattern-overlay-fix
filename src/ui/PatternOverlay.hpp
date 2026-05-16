#pragma once

class PlayLayer;

namespace cocos2d {
class CCTouch;
}

namespace ui {

/** In-game timeline overlay (toggle from pause → Pattern). */
void togglePatternOverlay(PlayLayer* playLayer);

/** Enable/disable pause-menu overlay repositioning. Saves position when disabled. */
void setPatternOverlayMoveMode(PlayLayer* playLayer, bool enabled);
void togglePatternOverlayMoveMode(PlayLayer* playLayer);
bool isPatternOverlayMoveMode();
bool beginPatternOverlayMove(PlayLayer* playLayer, cocos2d::CCTouch* touch);
void movePatternOverlay(PlayLayer* playLayer, cocos2d::CCTouch* touch);
void endPatternOverlayMove(PlayLayer* playLayer);

/** Remove overlay if present (e.g. level exit). */
void dismissPatternOverlay();

} // namespace ui
