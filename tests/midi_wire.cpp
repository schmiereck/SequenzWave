#include "midi/MidiWire.h"
#include <cassert>
#include <cstdio>

int main() {
    const auto on = midi::noteOnMessage(1, 60, 100);
    assert(on.bytes[0] == 0x90 && on.bytes[1] == 60 && on.bytes[2] == 100);
    const auto off = midi::noteOffMessage(16, 127);
    assert(off.bytes[0] == 0x8f && off.bytes[1] == 127 && off.bytes[2] == 0);
    std::puts("PASS: MIDI channel voice bytes");
}
