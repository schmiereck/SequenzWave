#include "storage/SettingsCodec.h"
#include <cassert>
#include <cstdio>

int main() {
    storage::Data original;
    original.bpm = 137;
    original.brightness = 2;
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
    assert(generation == 0xffffffffU && restored.bpm == 137 && restored.brightness == 2);
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
