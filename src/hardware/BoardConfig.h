#pragma once
#include <cstdint>

namespace board {
constexpr int kSda = 8;
constexpr int kScl = 7;
constexpr int kLcdMosi = 1;
constexpr int kLcdMiso = 2;
constexpr int kLcdClock = 5;
constexpr int kLcdDc = 3;
constexpr int kBacklight = 6;
// ESP_RXD / GPIO44: J8 pin 27 on V1, pin 28 on V2. UART1 TX; no UART0 boot text.
constexpr int kMidiTx = 44;
constexpr uint32_t kMidiBaud = 31250;
// Rev2.0 J8 pins 15/17. Reserved for sync only after OV5640 ribbon is unplugged.
constexpr int kSyncIn = 17;
constexpr int kSyncOut = 18;
constexpr uint8_t kBacklightPwmChannel = 0;
constexpr uint32_t kBacklightPwmHz = 20000;
constexpr uint8_t kDefaultBrightness = 10;
constexpr int kLcdCs = -1;  // Reference uses permanently selected LCD.
constexpr int kLcdReset = -1;  // Reset is TCA9554 P1, NOT an ESP GPIO.
constexpr uint8_t kExpanderAddress = 0x20;
constexpr uint8_t kResetExpanderPin = 1;
constexpr uint8_t kRotation = 1;
constexpr int16_t kNativeWidth = 320;
constexpr int16_t kNativeHeight = 480;
constexpr int16_t kWidth = 480;
constexpr int16_t kHeight = 320;
constexpr int32_t kSpiHz = 40000000;
}  // namespace board
