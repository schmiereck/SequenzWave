#pragma once
#include <array>
#include <cstdint>

namespace sequencer {
constexpr unsigned kStepCount = 16;
struct Step {
    uint8_t note = 60;  // MIDI C4 (middle C), independent of synth octave labels.
    uint8_t velocity = 100;
    uint8_t gate = 75;  // Percent of one sixteenth note.
    bool enabled = true;
};
struct Pattern {
    std::array<Step, kStepCount> steps{};
};
inline Pattern initialPattern() {
    Pattern result;
    const uint8_t melody[kStepCount] = {60, 62, 64, 67, 60, 64, 69, 67,
                                       60, 62, 65, 69, 67, 65, 64, 62};
    for (unsigned i = 0; i < kStepCount; ++i) result.steps[i].note = melody[i];
    return result;
}
}  // namespace sequencer
