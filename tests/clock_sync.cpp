#include "clocksync/ClockSync.h"
#include <cassert>
#include <cstdio>

int main() {
    clocksync::ClockSync clock;
    clocksync::Config config;
    assert(clock.configure(config));
    assert(clock.stepPeriodUs() == 125000);
    assert(!clock.acceptAnalogPulse(1000000));
    config.source = clocksync::Source::AnalogSync;
    assert(clock.configure(config));
    assert(!clock.signalPresent(1000000));
    assert(clock.acceptAnalogPulse(1000000));
    assert(clock.stepDeadlineAfterPulse(0) == 1000000);
    assert(clock.stepDeadlineAfterPulse(1) == 1125000); // 2 PPQN, two steps/pulse.
    assert(clock.stepDeadlineAfterPulse(2) == 0);
    assert(!clock.hasMeasuredTempo());
    assert(!clock.acceptAnalogPulse(1001000)); // Noise/debounce.
    assert(clock.acceptAnalogPulse(1250000));
    assert(clock.hasMeasuredTempo() && clock.estimatedBpm() == 120);
    assert(clock.stepDeadlineAfterPulse(1) == 1375000);
    assert(clock.signalPresent(1999999));
    assert(!clock.signalPresent(2000001)); // Three missed pulse intervals.
    assert(clock.acceptAnalogPulse(9000000)); // Long gap: reacquire, no false tempo.
    assert(!clock.hasMeasuredTempo() && clock.estimatedBpm() == 120);
    assert(clock.acceptAnalogPulse(9250000) && clock.hasMeasuredTempo());
    config.pulsesPerQuarter = 4;
    assert(clock.configure(config));
    assert(clock.acceptAnalogPulse(100000));
    assert(clock.stepsPerPulse() == 1 && clock.stepDeadlineAfterPulse(1) == 0);
    assert(clock.acceptAnalogPulse(225000));
    assert(clock.estimatedBpm() == 120);
    config.pulsesPerQuarter = 3;
    assert(!clock.configure(config));
    std::puts("PASS: internal period, analog 2/4 PPQN, interpolated step, debounce and loss");
}
