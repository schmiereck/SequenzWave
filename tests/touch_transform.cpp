#include "hardware/TouchTransform.h"
#include <cassert>
#include <cstdio>
#include <vector>

int main() {
    int16_t x = -1, y = -1;
    assert(board::toLandscape(0, 0, x, y) && x == 0 && y == 319);
    assert(board::toLandscape(319, 479, x, y) && x == 479 && y == 0);
    assert(board::toLandscape(319, 0, x, y) && x == 0 && y == 0);
    assert(board::toLandscape(0, 479, x, y) && x == 479 && y == 319);
    assert(!board::toLandscape(-1, 0, x, y));
    assert(!board::toLandscape(320, 0, x, y));
    assert(!board::toLandscape(0, -1, x, y));
    assert(!board::toLandscape(0, 480, x, y));
    std::vector<bool> seen(480 * 320, false);
    for (int16_t rawX = 0; rawX < 320; ++rawX) {
        for (int16_t rawY = 0; rawY < 480; ++rawY) {
            assert(board::toLandscape(rawX, rawY, x, y));
            assert(x >= 0 && x < 480 && y >= 0 && y < 320);
            const auto index = y * 480 + x;
            assert(!seen[index]);
            seen[index] = true;
        }
    }
    std::puts("PASS: corners, invalid input, all 153600 pixels bijective");
}
