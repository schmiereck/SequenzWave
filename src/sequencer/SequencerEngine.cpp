#include "SequencerEngine.h"

namespace sequencer {
namespace {
constexpr uint64_t kStepNumerator = 15000000;  // 60e6 / four sixteenths per beat.
}

uint64_t SequencerEngine::boundary(uint64_t ordinal) const {
    return epoch_ + ordinal * kStepNumerator / status_.bpm;
}

void SequencerEngine::release(uint64_t now) {
    if (noteActive_) {
        output_.noteOff(1, activeNote_, now);
        noteActive_ = false;
    }
}

void SequencerEngine::sound(uint64_t begin, uint64_t end, uint64_t now) {
    const Step& step = pattern_.steps[static_cast<unsigned>(status_.step)];
    offAt_ = begin + (end - begin) * step.gate / 100;
    // Do not emit stale notes after a scheduling stall.
    if (step.enabled && now < offAt_) {
        activeNote_ = step.note;
        noteActive_ = true;
        output_.noteOn(1, activeNote_, step.velocity, now);
    }
}

void SequencerEngine::start(uint64_t now) {
    if (status_.playing) return;
    status_.playing = true;
    status_.step = 0;
    status_.skippedSteps = 0;
    status_.maxLateUs = 0;
    epoch_ = now;
    ordinal_ = 0;
    sound(now, boundary(1), now);
}

void SequencerEngine::stop(uint64_t now) {
    release(now);  // Active pitch is remembered even if its step was edited.
    status_.playing = false;
    status_.step = -1;
}

void SequencerEngine::update(uint64_t now) {
    if (!status_.playing) return;
    if (noteActive_ && now >= offAt_) release(now);
    if (now < boundary(ordinal_ + 1)) return;

    // Inverse of floor(n * numerator / bpm), including fractional-us boundaries.
    const uint64_t due = ((now - epoch_ + 1) * status_.bpm - 1) / kStepNumerator;
    const uint64_t advance = due - ordinal_;
    status_.skippedSteps += static_cast<uint32_t>(advance - 1);
    status_.step = static_cast<int8_t>((status_.step + advance) % kStepCount);
    ordinal_ = due;
    const uint64_t late = now - boundary(due);
    if (late > status_.maxLateUs) status_.maxLateUs = late;
    release(now);
    sound(boundary(due), boundary(due + 1), now);
}

bool SequencerEngine::setStep(unsigned index, Step step) {
    if (index >= kStepCount || step.note > 127 || step.velocity == 0 ||
        step.velocity > 127 || step.gate < 5 || step.gate > 100) return false;
    pattern_.steps[index] = step;  // Edits take effect on the next visit.
    return true;
}

void SequencerEngine::setTempo(uint16_t bpm, uint64_t now) {
    if (bpm < 30 || bpm > 300 || bpm == status_.bpm) return;
    update(now);
    status_.bpm = bpm;
    // A tempo edit starts a fresh step interval; the sounding note keeps its gate.
    epoch_ = now;
    ordinal_ = 0;
}
}  // namespace sequencer
