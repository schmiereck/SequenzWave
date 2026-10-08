#pragma once
#include "AudioMode.h"
#include <cstdint>

namespace audio {
// Codec and I2S are initialized once on the UI task after board I2C setup.
// MIDI/timing callbacks only use a bounded, nonblocking queue.
bool begin(Mode initial);
bool setMode(Mode mode);
void noteOn(uint8_t note, uint8_t velocity);
void noteOff(uint8_t note);
void beat(bool accent);
bool ready();
uint32_t droppedEvents();
}
