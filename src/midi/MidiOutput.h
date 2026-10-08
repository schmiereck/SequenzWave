#pragma once
#include <cstdint>

namespace midi {
// Implementations must be bounded and nonblocking. Channel is 1..16.
class MidiOutput {
public:
    virtual ~MidiOutput() = default;
    virtual void noteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint64_t atUs) = 0;
    virtual void noteOff(uint8_t channel, uint8_t note, uint64_t atUs) = 0;
};
}  // namespace midi
