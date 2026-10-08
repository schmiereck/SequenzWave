#pragma once
#include <cstdint>

namespace clocksync {
enum class Source : uint8_t { Internal, AnalogSync, MidiClock };
enum class LossPolicy : uint8_t { Stop, Hold, FreeRun };

struct Config {
    Source source = Source::Internal;
    LossPolicy lossPolicy = LossPolicy::Stop;
    uint16_t bpm = 120;
    uint8_t pulsesPerQuarter = 2;
};

// Pure timing model. GPIO/ISR, transport commands and MIDI remain in adapters.
class ClockSync {
public:
    bool configure(Config config);
    Config config() const { return config_; }
    bool acceptAnalogPulse(uint64_t atUs);
    bool signalPresent(uint64_t nowUs) const;
    bool hasMeasuredTempo() const { return measured_; }
    uint16_t estimatedBpm() const;
    uint64_t stepPeriodUs() const;
    uint8_t stepsPerPulse() const { return 4 / config_.pulsesPerQuarter; }
    // Index 0 is the captured edge; index 1 is an interpolated step at 2 PPQN.
    uint64_t stepDeadlineAfterPulse(uint8_t index) const;
    uint64_t lastPulseUs() const { return lastPulseUs_; }

private:
    Config config_{};
    uint64_t lastPulseUs_ = 0;
    uint64_t pulsePeriodUs_ = 250000;
    bool seenPulse_ = false;
    bool measured_ = false;
};
}  // namespace clocksync
