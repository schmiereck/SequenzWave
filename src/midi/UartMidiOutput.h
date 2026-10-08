#pragma once
#include "MidiOutput.h"
#include "MidiWire.h"
#include <Arduino.h>

namespace midi {
// Timing-task-owned fixed ring. No allocation or USB logging in note callbacks.
class UartMidiOutput final : public MidiOutput {
public:
    bool begin();
    void noteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint64_t) override;
    void noteOff(uint8_t channel, uint8_t note, uint64_t) override;
    void drain();
    uint32_t droppedMessages() const { return dropped_; }

private:
    void enqueue(const Message& message);
    HardwareSerial port_{1};
    static constexpr uint8_t kCapacity = 96;
    uint8_t pending_[kCapacity] = {};
    uint8_t head_ = 0;
    uint8_t tail_ = 0;
    uint8_t count_ = 0;
    uint32_t dropped_ = 0;
    bool ready_ = false;
};
}  // namespace midi
