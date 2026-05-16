#include "PatternPopup.hpp"
#include "PatternOverlay.hpp"

#include <Geode/binding/PlayLayer.hpp>

using namespace geode::prelude;

namespace ui {

void showPatternPopup(PlayLayer* playLayer) {
    togglePatternOverlay(playLayer);
}

} // namespace ui
