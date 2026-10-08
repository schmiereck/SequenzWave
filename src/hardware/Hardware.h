#pragma once
#include <cstdint>

namespace hardware {
// Call once. Returns nullptr on success or a static diagnostic on failure.
const char* begin();
bool touchAvailable();
void service();
void setBrightness(uint8_t percent);
uint8_t brightness();
}  // namespace hardware
