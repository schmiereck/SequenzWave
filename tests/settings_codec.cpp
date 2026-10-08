#include "storage/SettingsCodec.h"
#include <cassert>
#include <cstdio>

int main() {
    storage::Data original;
    original.bpm = 137;
    original.brightness = 2;
    original.channel = 16;
    original.pattern.steps[0].note = 127;
    original.pattern.steps[0].velocity = 1;
    original.pattern.steps[0].gate = 5;
    original.pattern.steps[0].enabled = false;
    original.pattern.steps[15].note = 0;
    original.pattern.steps[15].velocity = 127;
    original.pattern.steps[15].gate = 100;
    const auto old = storage::encode(original, 0xffffffffU);
    storage::Data restored;
    uint32_t generation = 0;
    assert(storage::decode(old, restored, generation));
    assert(generation == 0xffffffffU && restored.bpm == 137 && restored.brightness == 2 && restored.channel == 16);
    assert(restored.pattern.steps[0].note == 127 && !restored.pattern.steps[0].enabled);
    assert(restored.pattern.steps[15].note == 0 && restored.pattern.steps[15].gate == 100);

    auto corrupt = old;
    for (size_t i = 0; i < storage::kRecordSize; ++i) {
        corrupt = old;
        corrupt[i] ^= 1;
        assert(!storage::decode(corrupt, restored, generation));
    }
    storage::Data invalid = original;
    invalid.pattern.steps[5].velocity = 0;
    assert(!storage::decode(storage::encode(invalid, 2), restored, generation));
    invalid = original;
    invalid.brightness = 0;
    assert(!storage::decode(storage::encode(invalid, 2), restored, generation));
    invalid = original;
    invalid.channel = 0;
    assert(!storage::decode(storage::encode(invalid, 2), restored, generation));

    // Existing V1 NVS slots survive the firmware update and default to channel 1.
    auto legacy = old;
    legacy[4] = 1;
    legacy[10] &= 1;
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < storage::kRecordSize - 4; ++i) {
        crc ^= legacy[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    crc = ~crc;
    for (unsigned i = 0; i < 4; ++i) legacy[76 + i] = crc >> (8 * i);
    assert(storage::decode(legacy, restored, generation) && restored.channel == 1);

    auto recent = original;
    recent.bpm = 140;
    const auto newer = storage::encode(recent, 0); // Generation wraps.
    bool selectedA = false;
    assert(storage::newest(old, newer, restored, generation, selectedA));
    assert(!selectedA && generation == 0 && restored.bpm == 140);
    assert(storage::newest(old, corrupt, restored, generation, selectedA));
    assert(selectedA && restored.bpm == 137);
    storage::Record blank{};
    assert(!storage::newest(blank, corrupt, restored, generation, selectedA));
    std::puts("PASS: versioned record, CRC, semantic validation, slot recovery and generation wrap");
}
