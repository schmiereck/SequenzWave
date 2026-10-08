#include <Arduino.h>
#include "hardware/Hardware.h"
#include "ui/TouchUI.h"
#include "app/SequencerRuntime.h"

namespace {
const char* startupError = nullptr;
uint32_t lastReport = 0;
}

void setup() {
    Serial.begin(115200);  // Native USB CDC; never wait for a connected PC.
    startupError = hardware::begin();
    if (!startupError && !app::begin()) startupError = "Sequencer task init failed";
    if (!startupError) ui::create(hardware::touchAvailable());
}

void loop() {
    hardware::service();
    if (!startupError) {
        ui::refresh();
        app::serviceDebug();
    }
    const uint32_t now = millis();
    if (now - lastReport >= 5000) {
        lastReport = now;
        if (startupError) Serial.printf("INIT ERROR: %s\n", startupError);
        else if (Serial && Serial.availableForWrite() >= 120) {
            const auto state = app::snapshot();
            Serial.printf("UI alive | touch=%s | PSRAM_heap=%u | heap=%u | late_us=%llu | skipped=%u | dropped=%u\n",
            hardware::touchAvailable() ? "ready" : "MISSING",
            ESP.getPsramSize(), ESP.getFreeHeap(),
            static_cast<unsigned long long>(state.transport.maxLateUs),
            state.transport.skippedSteps, state.droppedLogs);
        }
    }
    // Let the idle task run. Sequencer timing is owned by its separate task.
    vTaskDelay(pdMS_TO_TICKS(5) > 0 ? pdMS_TO_TICKS(5) : 1);
}
