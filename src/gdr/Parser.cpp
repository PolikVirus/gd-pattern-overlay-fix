#include "Parser.hpp"
#include "BinaryReader.hpp"

#include <algorithm>

namespace gdr {

namespace {

struct Unpacked {
    int delta;
    int button;
    bool down;
};

Unpacked unpackNonPlatformer(int packed) {
    return {packed >> 1, 1, (packed & 1) != 0};
}

Unpacked unpackPlatformer(int packed) {
    int btn = (packed >> 1) & 3;
    return {packed >> 3, btn == 0 ? 1 : btn, (packed & 1) != 0};
}

} // namespace

bool isGdr2(std::span<uint8_t const> bytes) {
    return bytes.size() >= 3 && bytes[0] == 'G' && bytes[1] == 'D' && bytes[2] == 'R';
}

bool isLikelyGdr1(std::span<uint8_t const> bytes) {
    if (bytes.empty()) return false;
    uint8_t b = bytes[0];
    return b == 0xde || b == 0xdf || b == 0xdc || b == 0xdd || b == 0x80 || b == 0x81;
}

Replay parseGdr2(std::vector<uint8_t> const& bytes) {
    BinaryReader reader(bytes);
    reader.readMagic("GDR");

    Replay replay;
    replay.format = ReplayFormat::Gdr2;

    (void)reader.readVarint(); // version
    std::string inputTag = reader.readString();
    (void)reader.readString(); // author
    (void)reader.readString(); // description
    replay.duration = reader.readFloat32();
    (void)reader.readVarint(); // gameVersion
    replay.framerate = reader.readFloat64();
    (void)reader.readVarint(); // seed
    (void)reader.readVarint(); // coins
    (void)reader.readBool();   // ldm
    replay.platformer = reader.readBool();
    replay.botName = reader.readString();
    replay.botVersion = reader.readVarint();
    replay.levelId = reader.readVarint();
    replay.levelName = reader.readString();

    int extensionSize = reader.readVarint();
    if (extensionSize > static_cast<int>(reader.remaining())) {
        throw std::runtime_error("Invalid extension size");
    }
    reader.skip(static_cast<size_t>(extensionSize));

    int deathCount = reader.readVarint();
    int deathBase = 0;
    for (int i = 0; i < deathCount; ++i) {
        deathBase += reader.readVarint();
        replay.deaths.push_back(deathBase);
    }

    int inputCount = reader.readVarint();
    int p1Remaining = reader.readVarint();
    int frameBase = 0;

    while (reader.remaining() > 0) {
        int packed = reader.readVarint();
        Unpacked chunk = replay.platformer ? unpackPlatformer(packed) : unpackNonPlatformer(packed);
        int frame = chunk.delta + frameBase;

        bool player2 = p1Remaining == 0;

        if (!inputTag.empty()) {
            int ext = reader.readVarint();
            if (ext > static_cast<int>(reader.remaining())) {
                throw std::runtime_error("Invalid input extension size");
            }
            reader.skip(static_cast<size_t>(ext));
        }

        replay.inputs.push_back({frame, chunk.button, player2, chunk.down});

        frameBase = frame;
        if (p1Remaining > 0) {
            --p1Remaining;
            if (p1Remaining == 0) frameBase = 0;
        }
    }

    (void)inputCount;
    std::stable_sort(replay.inputs.begin(), replay.inputs.end(), [](Input const& a, Input const& b) {
        return a.frame < b.frame;
    });
    return replay;
}

Replay parseReplay(std::vector<uint8_t> const& bytes) {
    if (isGdr2(bytes)) return parseGdr2(bytes);
    if (isLikelyGdr1(bytes)) {
        throw std::runtime_error(
            "xdBot .gdr (MessagePack) is not supported in v1.0. Export as .gdr2 or use the web app.");
    }
    throw std::runtime_error("Unknown replay format (expected .gdr2 with GDR header)");
}

} // namespace gdr
