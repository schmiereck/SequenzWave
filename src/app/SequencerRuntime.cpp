#include "SequencerRuntime.h"
#include <Arduino.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <cstdio>

namespace {
struct Event { uint64_t at; uint8_t channel, note, velocity; bool on; };
StaticQueue_t commandControl, eventControl;
uint8_t commandStorage[32 * sizeof(app::Command)];
uint8_t eventStorage[128 * sizeof(Event)];
QueueHandle_t commands = nullptr, events = nullptr;
StaticTask_t taskControl;
StackType_t taskStack[2048];
portMUX_TYPE snapshotLock = portMUX_INITIALIZER_UNLOCKED;
app::Snapshot shared;
uint32_t dropped = 0;  // Timing-task-owned, published through shared snapshot.

class MockMidiOutput final : public midi::MidiOutput {
    void noteOn(uint8_t channel, uint8_t note, uint8_t velocity, uint64_t at) override {
        emit({at, channel, note, velocity, true});
    }
    void noteOff(uint8_t channel, uint8_t note, uint64_t at) override {
        emit({at, channel, note, 0, false});
    }
    void emit(const Event& event) {
        if (xQueueSend(events, &event, 0) != pdTRUE) ++dropped;
    }
};
MockMidiOutput output;
sequencer::SequencerEngine engine(output);

void run(void*) {
    TickType_t wake = xTaskGetTickCount();
    for (;;) {
        app::Command command;
        // Bound processing so a stream of edits cannot starve clock handling.
        for (unsigned i = 0; i < 8 && xQueueReceive(commands, &command, 0) == pdTRUE; ++i) {
            const uint64_t now = esp_timer_get_time();
            switch (command.action) {
            case app::Action::Toggle:
                if (engine.status().playing) engine.stop(now); else engine.start(now);
                break;
            case app::Action::Start: engine.start(now); break;
            case app::Action::Stop: engine.stop(now); break;
            case app::Action::Tempo: engine.setTempo(command.value, now); break;
            case app::Action::Edit: engine.setStep(command.value, command.step); break;
            }
        }
        engine.update(esp_timer_get_time());
        app::Snapshot next;
        next.transport = engine.status();
        next.droppedLogs = dropped;
        portENTER_CRITICAL(&snapshotLock);
        shared = next;
        portEXIT_CRITICAL(&snapshotLock);
        vTaskDelayUntil(&wake, 1);  // One RTOS tick; no busy loop or UI dependency.
    }
}
}  // namespace

namespace app {
bool begin(const sequencer::Pattern& pattern, uint16_t bpm) {
    for (unsigned i = 0; i < sequencer::kStepCount; ++i) {
        if (!engine.setStep(i, pattern.steps[i])) return false;
    }
    engine.setTempo(bpm, 0);
    commands = xQueueCreateStatic(32, sizeof(Command), commandStorage, &commandControl);
    events = xQueueCreateStatic(128, sizeof(Event), eventStorage, &eventControl);
    return commands && events && xTaskCreateStaticPinnedToCore(
        run, "sequencer", sizeof(taskStack), nullptr, 3, taskStack, &taskControl, 0);
}
bool send(const Command& command) {
    return commands && xQueueSend(commands, &command, 0) == pdTRUE;
}
Snapshot snapshot() {
    portENTER_CRITICAL(&snapshotLock);
    const Snapshot result = shared;
    portEXIT_CRITICAL(&snapshotLock);
    return result;
}
void serviceDebug() {
    if (!events) return;
    // Optional bench controls: p = play, s = stop. No synth data received.
    for (unsigned i = 0; i < 8 && Serial.available(); ++i) {
        const int key = Serial.read();
        Command command;
        if (key == 'p' || key == 's') {
            command.action = key == 'p' ? Action::Start : Action::Stop;
            if (!send(command)) Serial.println("Command queue full");
        }
    }
    // Discard debug records when no monitor is connected. The sequencer keeps running.
    for (unsigned i = 0; i < 8; ++i) {
        Event event;
        if (!Serial) {
            if (xQueueReceive(events, &event, 0) != pdTRUE) break;
            continue;
        }
        if (Serial.availableForWrite() < 100) break;
        if (xQueueReceive(events, &event, 0) != pdTRUE) break;
        char line[100];
        const int size = snprintf(line, sizeof(line), "MOCK t=%llu %s ch=%u note=%u vel=%u\n",
            static_cast<unsigned long long>(event.at), event.on ? "ON" : "OFF",
            event.channel, event.note, event.velocity);
        if (size > 0 && size < static_cast<int>(sizeof(line))) {
            Serial.write(reinterpret_cast<const uint8_t*>(line), size);
        }
    }
}
}  // namespace app
