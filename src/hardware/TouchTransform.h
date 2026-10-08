#pragma once
#include "BoardConfig.h"

namespace board {
// Inverse of ST7796 rotation 1 (MX | MV), relative to portrait rotation 0.
inline bool toLandscape(int16_t rawX, int16_t rawY, int16_t& x, int16_t& y) {
    if (rawX < 0 || rawX >= kNativeWidth || rawY < 0 || rawY >= kNativeHeight) {
        return false;
    }
    x = rawY;
    y = kNativeWidth - 1 - rawX;
    return true;
}
}  // namespace board
