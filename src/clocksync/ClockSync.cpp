#include "ClockSync.h"

namespace clocksync {
bool ClockSync::configure(Config config) {
    if (config.bpm < 30 || config.bpm > 300 ||
        (config.pulsesPerQuarter != 2 && config.pulsesPerQuarter != 4)) return false;
    config_ = config;
    pulsePeriodUs_ = 60000000ULL / (config.bpm * config.pulsesPerQuarter);
    seenPulse_ = false;
    measured_ = false;
    lastPulseUs_ = 0;
    return true;
}

bool ClockSync::acceptAnalogPulse(uint64_t atUs) {
    if (config_.source != Source::AnalogSync) return false;
    if (seenPulse_) {
        if (atUs <= lastPulseUs_ || atUs - lastPulseUs_ < 3000) return false;
        const uint64_t interval = atUs - lastPulseUs_;
        const uint64_t minPeriod = 60000000ULL / (600 * config_.pulsesPerQuarter);
        const uint64_t maxPeriod = 60000000ULL / (10 * config_.pulsesPerQuarter);
        if (interval >= minPeriod && interval <= maxPeriod) {
            pulsePeriodUs_ = interval;
            measured_ = true;
        } else {
            // A new first edge after a long gap must not become a false low BPM.
            measured_ = false;
        }
    }
    lastPulseUs_ = atUs;
    seenPulse_ = true;
    return true;
}

bool ClockSync::signalPresent(uint64_t nowUs) const {
    if (config_.source != Source::AnalogSync || !seenPulse_ || nowUs < lastPulseUs_) return false;
    return nowUs - lastPulseUs_ <= 3 * pulsePeriodUs_;
}

uint16_t ClockSync::estimatedBpm() const {
    const uint64_t denominator = pulsePeriodUs_ * config_.pulsesPerQuarter;
    return static_cast<uint16_t>((60000000ULL + denominator / 2) / denominator);
}

uint64_t ClockSync::stepPeriodUs() const {
    if (config_.source == Source::AnalogSync)
        return pulsePeriodUs_ / stepsPerPulse();
    return 15000000ULL / config_.bpm;
}

uint64_t ClockSync::stepDeadlineAfterPulse(uint8_t index) const {
    if (config_.source != Source::AnalogSync || !seenPulse_ || index >= stepsPerPulse())
        return 0;
    return lastPulseUs_ + index * pulsePeriodUs_ / stepsPerPulse();
}
}  // namespace clocksync
