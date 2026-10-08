#pragma once
#include "sequencer/PatternModel.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace storage {
constexpr size_t kRecordSize = 80;
struct Data {
    sequencer::Pattern pattern = sequencer::initialPattern();
    uint16_t bpm = 120;
    uint8_t brightness = 10;
    uint8_t channel = 1;
};
using Record = std::array<uint8_t, kRecordSize>;
Record encode(const Data& data, uint32_t generation);
bool decode(const Record& record, Data& data, uint32_t& generation);
bool newest(const Record& a, const Record& b, Data& data,
            uint32_t& generation, bool& selectedA);
}  // namespace storage
