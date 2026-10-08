#include "UartMidiOutput.h"
#include "hardware/BoardConfig.h"
#include <driver/uart.h>

namespace midi {
bool UartMidiOutput::begin() {
    port_.begin(board::kMidiBaud, SERIAL_8N1, -1, board::kMidiTx);
    ready_ = uart_is_driver_installed(UART_NUM_1);
    return ready_;
}
void UartMidiOutput::enqueue(const Message& message) {
    if (!ready_ || kCapacity - count_ < 3) { ++dropped_; return; }
    for (uint8_t byte : message.bytes) {
        pending_[head_] = byte;
        head_ = static_cast<uint8_t>((head_ + 1) % kCapacity);
        ++count_;
    }
}
void UartMidiOutput::noteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint64_t) {
    enqueue(noteOnMessage(channel, note, velocity));
}
void UartMidiOutput::noteOff(uint8_t channel, uint8_t note, uint64_t) {
    enqueue(noteOffMessage(channel, note));
}
void UartMidiOutput::drain() {
    if (!ready_ || !count_) return;
    const uint8_t contiguous = head_ > tail_ ? head_ - tail_ : kCapacity - tail_;
    const uint8_t amount = contiguous < count_ ? contiguous : count_;
    const int written = uart_tx_chars(UART_NUM_1,
        reinterpret_cast<const char*>(pending_ + tail_), amount);
    if (written > 0) {
        tail_ = static_cast<uint8_t>((tail_ + written) % kCapacity);
        count_ -= static_cast<uint8_t>(written);
    }
}
}  // namespace midi
