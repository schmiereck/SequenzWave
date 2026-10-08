#include "SettingsCodec.h"

namespace storage {
namespace {
uint32_t checksum(const Record& record, size_t dataSize) {
    uint32_t crc = 0xffffffffU;
    for (size_t i = 0; i < dataSize; ++i) {
        crc ^= record[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xedb88320U & (0U - (crc & 1U)));
        }
    }
    return ~crc;
}
void write32(Record& record, size_t index, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) record[index + i] = value >> (8 * i);
}
uint32_t read32(const Record& record, size_t index) {
    uint32_t result = 0;
    for (unsigned i = 0; i < 4; ++i) result |= uint32_t(record[index + i]) << (8 * i);
    return result;
}
}  // namespace

Record encode(const Data& data, uint32_t generation) {
    Record record{};
    record[0] = 'S'; record[1] = 'W'; record[2] = 'V'; record[3] = '2';
    record[4] = 4;  // V4 adds speaker volume after the sixteen steps.
    write32(record, 5, generation);
    record[9] = static_cast<uint8_t>(data.bpm);
    const bool validChannel = data.channel >= 1 && data.channel <= 16;
    record[10] = static_cast<uint8_t>((data.bpm >> 8) |
        ((validChannel ? data.channel - 1 : 0) << 4) |
        ((static_cast<uint8_t>(data.audioMode) & 3) << 1) |
        (validChannel ? 0 : 0x08));
    record[11] = data.brightness;
    for (unsigned i = 0; i < sequencer::kStepCount; ++i) {
        const auto& step = data.pattern.steps[i];
        const size_t pos = 12 + 4 * i;
        record[pos] = step.note;
        record[pos + 1] = step.velocity;
        record[pos + 2] = step.gate;
        record[pos + 3] = step.enabled ? 1 : 0;
    }
    record[76] = data.speakerVolume;
    write32(record, 80, checksum(record, 80));
    return record;
}

bool decode(const Record& record, Data& data, uint32_t& generation) {
    if (record[0] != 'S' || record[1] != 'W' || record[2] != 'V' ||
        record[3] != '2' || record[4] < 1 || record[4] > 4) return false;
    const bool current = record[4] == 4;
    if (read32(record, current ? 80 : 76) != checksum(record, current ? 80 : 76)) return false;
    Data candidate;
    if ((record[4] >= 3 && ((record[10] & 0x08) || ((record[10] >> 1) & 3) > 2)) ||
        (record[4] == 2 && (record[10] & 0x0e)) ||
        (record[4] == 1 && record[10] > 1)) return false;
    candidate.bpm = uint16_t(record[9]) | (uint16_t(record[10] & 1) << 8);
    candidate.brightness = record[11];
    candidate.channel = record[4] >= 2 ? (record[10] >> 4) + 1 : 1;
    candidate.audioMode = record[4] >= 3 ?
        static_cast<audio::Mode>((record[10] >> 1) & 3) : audio::Mode::Off;
    candidate.speakerVolume = current ? record[76] : 80;
    if (candidate.bpm < 30 || candidate.bpm > 300 ||
        candidate.brightness < 2 || candidate.brightness > 100 ||
        candidate.speakerVolume > 100 ||
        (current && (record[77] || record[78] || record[79]))) return false;
    for (unsigned i = 0; i < sequencer::kStepCount; ++i) {
        const size_t pos = 12 + 4 * i;
        auto& step = candidate.pattern.steps[i];
        step.note = record[pos];
        step.velocity = record[pos + 1];
        step.gate = record[pos + 2];
        step.enabled = record[pos + 3] == 1;
        if (step.note > 127 || step.velocity == 0 || step.velocity > 127 ||
            step.gate < 5 || step.gate > 100 || record[pos + 3] > 1) return false;
    }
    data = candidate;
    generation = read32(record, 5);
    return true;
}

bool newest(const Record& a, const Record& b, Data& data,
            uint32_t& generation, bool& selectedA) {
    Data first, second;
    uint32_t firstGeneration = 0, secondGeneration = 0;
    const bool validA = decode(a, first, firstGeneration);
    const bool validB = decode(b, second, secondGeneration);
    if (validA && (!validB || static_cast<int32_t>(firstGeneration - secondGeneration) > 0)) {
        data = first; generation = firstGeneration; selectedA = true;
        return true;
    }
    if (validB) {
        data = second; generation = secondGeneration; selectedA = false;
        return true;
    }
    return false;
}
}  // namespace storage
