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
