#pragma once

namespace hardware {
// Call once. Returns nullptr on success or a static diagnostic on failure.
const char* begin();
bool touchAvailable();
void service();
}  // namespace hardware
