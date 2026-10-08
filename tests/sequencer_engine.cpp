#include "sequencer/SequencerEngine.h"
#include <cassert>
#include <cstdio>
#include <vector>

struct Event { bool on; uint8_t note, velocity; uint64_t at; };
struct RecordingOutput : midi::MidiOutput {
    std::vector<Event> events;
    void noteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint64_t at) override {
        assert(channel == 1);
        events.push_back({true, note, velocity, at});
    }
    void noteOff(uint8_t channel, uint8_t note, uint64_t at) override {
        assert(channel == 1);
        events.push_back({false, note, 0, at});
    }
};

int main() {
    using sequencer::SequencerEngine;
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        engine.update(100);
        assert(out.events.empty());
        engine.start(1000);
        engine.start(2000);  // Idempotent: do not retrigger a held note.
        assert(out.events.size() == 1 && out.events[0].note == 60);
        engine.update(94749);
        assert(out.events.size() == 1);
        engine.update(94750);  // 75% of 125000 us, plus epoch.
        assert(out.events.size() == 2 && !out.events.back().on);
        engine.update(126000);
        assert(engine.status().step == 1 && out.events.back().note == 62);
        engine.stop(126100);
        assert(!out.events.back().on && out.events.back().note == 62);
        const auto count = out.events.size();
        engine.stop(126200);
        engine.update(900000);
        assert(out.events.size() == count && engine.status().step == -1);
        engine.start(1000000);
        assert(engine.status().step == 0 && out.events.back().note == 60);
    }
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        sequencer::Step step;
        step.gate = 100;
        step.note = 127;
        step.velocity = 1;
        assert(engine.setStep(0, step));
        engine.start(0);
        step.note = 0;
        assert(engine.setStep(0, step));
        engine.update(125000);
        assert(out.events.size() == 3);
        assert(!out.events[1].on && out.events[1].note == 127); // Original active pitch.
        assert(out.events[2].on && out.events[2].note == 62);
        engine.stop(125001);
        engine.start(125002);
        assert(out.events.back().note == 0 && out.events.back().velocity == 1);
    }
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        sequencer::Step rest;
        rest.enabled = false;
        engine.setStep(0, rest);
        engine.start(0);
        assert(out.events.empty());
        engine.update(125000);
        assert(out.events.size() == 1 && out.events[0].note == 62);
    }
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        engine.start(0);
        engine.update(1010000); // Skip steps 1..7, no burst of stale events.
        assert(engine.status().step == 8 && engine.status().skippedSteps == 7);
        assert(out.events.size() == 3 && out.events.back().at == 1010000);
        engine.update(1999999); // Gate already elapsed: release only, no stale ON.
        assert(engine.status().step == 15 && !out.events.back().on);
        engine.update(2000000);
        assert(engine.status().step == 0 && out.events.back().on);
    }
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        engine.start(0);
        engine.setTempo(60, 50000);
        engine.update(93750); // Existing note-off deadline is preserved.
        assert(!out.events.back().on);
        engine.update(299999);
        assert(engine.status().step == 0);
        engine.update(300000);
        assert(engine.status().step == 1 && out.events.back().at == 300000);
        engine.setTempo(0, 300000);
        engine.setTempo(301, 300000);
        assert(engine.status().bpm == 60);
    }
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        const uint64_t epoch = 0xffffffffULL - 50; // Cross a 32-bit microsecond wrap.
        engine.setTempo(137, epoch);
        engine.start(epoch);
        for (uint64_t n = 1; n <= 10000; ++n) {
            const uint64_t at = epoch + n * 15000000 / 137;
            engine.update(at - 1);
            assert(engine.status().step == static_cast<int>((n - 1) % 16));
            engine.update(at);
            assert(engine.status().step == static_cast<int>(n % 16));
            assert(out.events.back().on && out.events.back().at == at);
        }
        assert(engine.status().skippedSteps == 0 && engine.status().maxLateUs == 0);
    }
    {
        RecordingOutput out;
        SequencerEngine engine(out);
        sequencer::Step invalid;
        assert(!engine.setStep(16, invalid));
        invalid.note = 128;
        assert(!engine.setStep(0, invalid));
        invalid.note = 60;
        invalid.velocity = 0;
        assert(!engine.setStep(0, invalid));
        invalid.velocity = 100;
        invalid.gate = 0;
        assert(!engine.setStep(0, invalid));
        invalid.gate = 101;
        assert(!engine.setStep(0, invalid));
    }
    std::puts("PASS: transport, gates, edits, rests, tempo, stalls, wrap, 10000 fractional steps");
}
