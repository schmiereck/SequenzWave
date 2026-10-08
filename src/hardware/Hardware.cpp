#include "Hardware.h"
#include "BoardConfig.h"
#include "TouchTransform.h"
#include <Arduino.h>
#include <Arduino_GFX_Library.h>
#include <TCA9554.h>
#include <TouchDrvFT6X36.hpp>
#include <Wire.h>
#include <lvgl.h>
#include <esp32s3/spiram.h>

namespace {
Arduino_ESP32SPI bus(board::kLcdDc, board::kLcdCs, board::kLcdClock,
                     board::kLcdMosi, board::kLcdMiso);
Arduino_ST7796 display(&bus, board::kLcdReset, board::kRotation, true,
                       board::kNativeWidth, board::kNativeHeight);
TCA9554 expander(board::kExpanderAddress);
TouchDrvFT6X36 touch;
bool hasTouch = false;
bool ready = false;
uint8_t lightPercent = board::kDefaultBrightness;
uint32_t lastTick = 0;
// Synchronous SPI flush: one small internal-RAM buffer is sufficient.
lv_color_t pixels[board::kWidth * 20];
lv_disp_draw_buf_t drawBuffer;
lv_disp_drv_t displayDriver;
lv_indev_drv_t inputDriver;

void flush(lv_disp_drv_t* driver, const lv_area_t* area, lv_color_t* colors) {
    display.draw16bitRGBBitmap(area->x1, area->y1,
        reinterpret_cast<uint16_t*>(colors), area->x2 - area->x1 + 1,
        area->y2 - area->y1 + 1);
    lv_disp_flush_ready(driver);  // Transfer has completed; no async DMA.
}

void readTouch(lv_indev_drv_t*, lv_indev_data_t* data) {
    data->state = LV_INDEV_STATE_REL;
    int16_t rawX = 0, rawY = 0, x = 0, y = 0;
    if (hasTouch && touch.getPoint(&rawX, &rawY, 1) &&
        board::toLandscape(rawX, rawY, x, y)) {
        data->point.x = x;
        data->point.y = y;
        data->state = LV_INDEV_STATE_PR;
    }
}
}  // namespace

namespace hardware {
const char* begin() {
    static_assert(sizeof(lv_color_t) == 2, "RGB565 required");
    pinMode(board::kBacklight, OUTPUT);
    digitalWrite(board::kBacklight, LOW);
    // Arduino getPsramSize() reports heap capacity, reduced by allocator overhead.
    // The IDF API reports the physical chip size required by this board profile.
    if (!psramFound() || esp_spiram_get_size() != 8U * 1024U * 1024U) {
        return "8 MB OPI PSRAM not detected; check qio_opi configuration";
    }
    if (ESP.getFlashChipSize() != 16U * 1024U * 1024U) {
        return "Expected 16 MB flash; verify board variant";
    }
    if (!Wire.begin(board::kSda, board::kScl, 400000)) return "I2C init failed";
    Wire.setTimeOut(20);
    if (!expander.begin() ||
        !expander.pinMode1(board::kResetExpanderPin, OUTPUT) ||
        !expander.pinMode1(board::kAmpEnableExpanderPin, OUTPUT) ||
        !expander.write1(board::kAmpEnableExpanderPin, LOW)) {
        return "TCA9554 at 0x20 missing";
    }
    // Board-specific startup timing from the Waveshare reference.
    if (!expander.write1(board::kResetExpanderPin, HIGH)) return "LCD reset I2C error";
    delay(10);
    if (!expander.write1(board::kResetExpanderPin, LOW)) return "LCD reset I2C error";
    delay(10);
    if (!expander.write1(board::kResetExpanderPin, HIGH)) return "LCD reset I2C error";
    delay(200);
    hasTouch = touch.begin(Wire, FT6X36_SLAVE_ADDRESS);
    if (!display.begin(board::kSpiHz)) return "ST7796 bus init failed";
    if (display.width() != board::kWidth || display.height() != board::kHeight) {
        return "Unexpected display orientation";
    }
    display.fillScreen(0);
    lv_init();
    lv_disp_draw_buf_init(&drawBuffer, pixels, nullptr, board::kWidth * 20);
    lv_disp_drv_init(&displayDriver);
    displayDriver.hor_res = board::kWidth;
    displayDriver.ver_res = board::kHeight;
    displayDriver.draw_buf = &drawBuffer;
    displayDriver.flush_cb = flush;
    if (!lv_disp_drv_register(&displayDriver)) return "LVGL display allocation failed";
    lv_indev_drv_init(&inputDriver);
    inputDriver.type = LV_INDEV_TYPE_POINTER;
    inputDriver.read_cb = readTouch;
    if (!lv_indev_drv_register(&inputDriver)) return "LVGL input allocation failed";
    lastTick = millis();
    ready = true;
    if (!ledcSetup(board::kBacklightPwmChannel, board::kBacklightPwmHz, 8)) {
        return "Backlight PWM init failed";
    }
    ledcAttachPin(board::kBacklight, board::kBacklightPwmChannel);
    setBrightness(board::kDefaultBrightness);
    return nullptr;
}

bool touchAvailable() { return hasTouch; }

void setBrightness(uint8_t percent) {
    lightPercent = percent < 2 ? 2 : (percent > 100 ? 100 : percent);
    ledcWrite(board::kBacklightPwmChannel, (lightPercent * 255U + 50U) / 100U);
}
uint8_t brightness() { return lightPercent; }
bool setSpeakerAmplifier(bool enabled) {
    return expander.write1(board::kAmpEnableExpanderPin, enabled ? HIGH : LOW);
}

void service() {
    if (!ready) return;
    const uint32_t now = millis();
    lv_tick_inc(now - lastTick);  // Unsigned subtraction handles millis wrap.
    lastTick = now;
    lv_timer_handler();
}
}  // namespace hardware
