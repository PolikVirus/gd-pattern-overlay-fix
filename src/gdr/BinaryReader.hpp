#pragma once

#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace gdr {

class BinaryReader {
public:
    explicit BinaryReader(std::vector<uint8_t> bytes)
        : m_bytes(std::move(bytes)) {}

    size_t remaining() const { return m_bytes.size() - m_offset; }

    void skip(size_t n) {
        if (n > remaining()) m_offset = m_bytes.size();
        else m_offset += n;
    }

    bool readBool() { return readU8() != 0; }

    uint8_t readU8() {
        if (remaining() < 1) throw std::runtime_error("Unexpected end of file");
        return m_bytes[m_offset++];
    }

    std::vector<uint8_t> readBytes(size_t n) {
        if (n > remaining()) throw std::runtime_error("Unexpected end of file");
        std::vector<uint8_t> out(m_bytes.begin() + static_cast<ptrdiff_t>(m_offset),
                                 m_bytes.begin() + static_cast<ptrdiff_t>(m_offset + n));
        m_offset += n;
        return out;
    }

    float readFloat32() {
        auto raw = readBytes(4);
        uint32_t bits = 0;
        std::memcpy(&bits, raw.data(), 4);
        // big-endian wire
        bits = ((bits & 0x000000FFu) << 24) | ((bits & 0x0000FF00u) << 8) |
               ((bits & 0x00FF0000u) >> 8) | ((bits & 0xFF000000u) >> 24);
        float v;
        std::memcpy(&v, &bits, 4);
        return v;
    }

    double readFloat64() {
        auto raw = readBytes(8);
        uint64_t bits = 0;
        std::memcpy(&bits, raw.data(), 8);
        bits = ((bits & 0x00000000000000FFull) << 56) |
               ((bits & 0x000000000000FF00ull) << 40) |
               ((bits & 0x0000000000FF0000ull) << 24) |
               ((bits & 0x00000000FF000000ull) << 8) |
               ((bits & 0x000000FF00000000ull) >> 8) |
               ((bits & 0x0000FF0000000000ull) >> 24) |
               ((bits & 0x00FF000000000000ull) >> 40) |
               ((bits & 0xFF00000000000000ull) >> 56);
        double v;
        std::memcpy(&v, &bits, 8);
        return v;
    }

    int readVarint() {
        int result = 0;
        for (int i = 0; i <= 8; ++i) {
            if (remaining() < 1) throw std::runtime_error("Unexpected end of file");
            uint8_t byte = m_bytes[m_offset++];
            result |= static_cast<int>(byte & 0x7f) << (i * 7);
            if ((byte & 0x80) == 0) break;
        }
        return result;
    }

    std::string readString() {
        int len = readVarint();
        if (len > 0xffff) throw std::runtime_error("String too long");
        auto data = readBytes(static_cast<size_t>(len));
        return std::string(reinterpret_cast<char const*>(data.data()), data.size());
    }

    void readMagic(std::string_view expected) {
        auto bytes = readBytes(expected.size());
        std::string actual(reinterpret_cast<char const*>(bytes.data()), bytes.size());
        if (actual != expected) {
            throw std::runtime_error("Invalid magic: expected GDR");
        }
    }

private:
    std::vector<uint8_t> m_bytes;
    size_t m_offset = 0;
};

} // namespace gdr
