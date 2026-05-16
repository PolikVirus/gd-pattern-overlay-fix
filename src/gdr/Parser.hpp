#pragma once

#include "Types.hpp"
#include <span>
#include <vector>

namespace gdr {

/** Parse .gdr2 (GDR magic). Throws on .gdr1 xdBot (MessagePack) with a clear message. */
Replay parseReplay(std::vector<uint8_t> const& bytes);

bool isGdr2(std::span<uint8_t const> bytes);
bool isLikelyGdr1(std::span<uint8_t const> bytes);

} // namespace gdr
