#include <Arduino.h>
#include "hardware/Hardware.h"
#include "ui/TouchUI.h"
#include "app/SequencerRuntime.h"
#include "storage/SettingsStore.h"
#include "audio/Speaker.h"

namespace {
const char* startupError = nullptr;
uint32_t lastReport = 0;
storage::Data loaded;
}

void setup() {
    Serial.begin(115200);  // Native USB CDC; never wait for a connected PC.
    startupError = hardware::begin();
    if (!startupError) {
        storage::begin(loaded); // A corrupt/missing record loads safe defaults.
        hardware::setBrightness(loaded.brightness);
        if (!audio::begin(loaded.audioMode)) Serial.println("Speaker init failed; MIDI/UI continue");
        if (!app::begin(loaded.pattern, loaded.bpm, loaded.channel)) startupError = "Sequencer/UART init failed";
    }
    if (!startupError) ui::create(hardware::touchAvailable(), loaded);
}

void loop() {
    hardware::service();
    if (!startupError) {
        storage::service(millis());
        ui::refresh();
        app::serviceDebug();
    }
    const uint32_t now = millis();
    if (now - lastReport >= 5000) {
        lastReport = now;
        if (startupError) Serial.printf("INIT ERROR: %s\n", startupError);
        else if (Serial && Serial.availableForWrite() >= 120) {
            const auto state = app::snapshot();
            Serial.printf("UI alive | touch=%s | PSRAM_heap=%u | heap=%u | nvs=%s | late_us=%llu | skipped=%u | dropped=%u | midi_drop=%u | audio=%s | audio_drop=%u\n",
            hardware::touchAvailable() ? "ready" : "MISSING",
            ESP.getPsramSize(), ESP.getFreeHeap(),
            storage::loadedFromNvs() ? "loaded" : "defaults",
            static_cast<unsigned long long>(state.transport.maxLateUs),
            state.transport.skippedSteps, state.droppedLogs, state.droppedMidi,
            audio::ready() ? "ready" : "ERROR", audio::droppedEvents());
        }
    }
    // Let the idle task run. Sequencer timing is owned by its separate task.
    vTaskDelay(pdMS_TO_TICKS(5) > 0 ? pdMS_TO_TICKS(5) : 1);
}
