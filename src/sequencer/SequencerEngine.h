#pragma once
#include "PatternModel.h"
#include "midi/MidiOutput.h"

namespace sequencer {
struct Status {
    bool playing = false;
    int8_t step = -1;
    uint16_t bpm = 120;
    uint32_t skippedSteps = 0;
    uint64_t maxLateUs = 0;
};

// Single owner. Times are monotonic microseconds, supplied by the caller.
class SequencerEngine {
public:
    explicit SequencerEngine(midi::MidiOutput& output) : output_(output) {}
    void start(uint64_t now);
    void stop(uint64_t now);
    void update(uint64_t now);
    bool setStep(unsigned index, Step step);
    void setTempo(uint16_t bpm, uint64_t now);
    void setChannel(uint8_t channel, uint64_t now);
    Status status() const { return status_; }

private:
    void release(uint64_t now);
    void sound(uint64_t boundary, uint64_t end, uint64_t now);
    uint64_t boundary(uint64_t ordinal) const;
    midi::MidiOutput& output_;
    Pattern pattern_ = initialPattern();
    Status status_;
    uint64_t epoch_ = 0;
    uint64_t ordinal_ = 0;
    uint64_t offAt_ = 0;
    uint8_t activeNote_ = 0;
    uint8_t activeChannel_ = 1;
    uint8_t channel_ = 1;
    bool noteActive_ = false;
};
}  // namespace sequencer
