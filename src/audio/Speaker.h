#pragma once
#include "AudioMode.h"
#include <cstdint>

namespace audio {
// Codec and I2S are initialized once on the UI task after board I2C setup.
// MIDI/timing callbacks only use a bounded, nonblocking queue.
bool begin(Mode initial, uint8_t volume);
bool setMode(Mode mode);
// UI task only: 0..100 maps to DAC attenuation up to 0 dB (no codec boost).
bool setVolume(uint8_t volume);
void noteOn(uint8_t note, uint8_t velocity);
void noteOff(uint8_t note);
void beat(bool accent);
bool ready();
uint32_t droppedEvents();
}
