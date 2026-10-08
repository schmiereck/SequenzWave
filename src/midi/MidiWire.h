#pragma once
#include <cstdint>

namespace midi {
struct Message {
    uint8_t bytes[3];
};
inline Message noteOnMessage(uint8_t channel, uint8_t note, uint8_t velocity) {
    return {{static_cast<uint8_t>(0x90 | (channel - 1)), note, velocity}};
}
inline Message noteOffMessage(uint8_t channel, uint8_t note) {
    return {{static_cast<uint8_t>(0x80 | (channel - 1)), note, 0}};
}
}  // namespace midi
