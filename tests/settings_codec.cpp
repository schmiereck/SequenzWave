#include "storage/SettingsCodec.h"
#include <cassert>
#include <cstdio>

static storage::Record legacyRecord(storage::Record record, uint8_t version) {
    record[4] = version;
    if (version == 1) record[10] &= 1;
    else if (version == 2) record[10] &= static_cast<uint8_t>(~0x06);
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < 76; ++i) {
        crc ^= record[i];
        for (unsigned bit = 0; bit < 8; ++bit)
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
    }
    crc = ~crc;
    for (unsigned i = 0; i < 4; ++i) record[76 + i] = crc >> (8 * i);
    for (unsigned i = 80; i < storage::kRecordSize; ++i) record[i] = 0;
    return record;
}

int main() {
    storage::Data original;
    original.bpm = 137;
    original.brightness = 2;
    original.channel = 16;
    original.audioMode = audio::Mode::Notes;
    original.speakerVolume = 92;
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
    assert(generation == 0xffffffffU && restored.bpm == 137 && restored.brightness == 2 && restored.channel == 16 && restored.audioMode == audio::Mode::Notes && restored.speakerVolume == 92);
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
    invalid = original;
    invalid.audioMode = static_cast<audio::Mode>(3);
    assert(!storage::decode(storage::encode(invalid, 2), restored, generation));
    invalid = original;
    invalid.speakerVolume = 101;
    assert(!storage::decode(storage::encode(invalid, 2), restored, generation));

    // Existing V1 NVS slots survive the firmware update and default to channel 1.
    const auto legacy = legacyRecord(old, 1);
    assert(storage::decode(legacy, restored, generation) && restored.channel == 1);
    assert(restored.audioMode == audio::Mode::Off && restored.speakerVolume == 80);

    // V2 stored MIDI channel but had no speaker setting.
    const auto v2 = legacyRecord(old, 2);
    assert(storage::decode(v2, restored, generation));
    assert(restored.channel == 16 && restored.audioMode == audio::Mode::Off && restored.speakerVolume == 80);
    const auto v3 = legacyRecord(old, 3);
    assert(storage::decode(v3, restored, generation));
    assert(restored.channel == 16 && restored.audioMode == audio::Mode::Notes && restored.speakerVolume == 80);

    auto recent = original;
    recent.bpm = 140;
    const auto newer = storage::encode(recent, 0); // Generation wraps.
    bool selectedA = false;
    assert(storage::newest(old, newer, restored, generation, selectedA));
    assert(!selectedA && generation == 0 && restored.bpm == 140);
    assert(storage::newest(old, corrupt, restored, generation, selectedA));
    assert(selectedA && restored.bpm == 137);
    storage::Record blank{};
    assert(storage::newest(v3, blank, restored, generation, selectedA));
    assert(selectedA && restored.audioMode == audio::Mode::Notes && restored.speakerVolume == 80);
    assert(!storage::newest(blank, corrupt, restored, generation, selectedA));
    std::puts("PASS: versioned record, CRC, semantic validation, slot recovery and generation wrap");
}
