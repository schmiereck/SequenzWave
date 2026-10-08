#include <Arduino.h>
#include "hardware/Hardware.h"
#include "ui/TouchUI.h"

namespace {
const char* startupError = nullptr;
uint32_t lastReport = 0;
}

void setup() {
    Serial.begin(115200);  // Native USB CDC; never wait for a connected PC.
    startupError = hardware::begin();
    if (!startupError) ui::create(hardware::touchAvailable());
}

void loop() {
    hardware::service();
    const uint32_t now = millis();
    if (now - lastReport >= 5000) {
        lastReport = now;
        if (startupError) Serial.printf("INIT ERROR: %s\n", startupError);
        else Serial.printf("UI alive | touch=%s | PSRAM_heap=%u | heap=%u\n",
            hardware::touchAvailable() ? "ready" : "MISSING",
            ESP.getPsramSize(), ESP.getFreeHeap());
    }
    yield();
}
