#include "Speaker.h"
#include "hardware/BoardConfig.h"
#include "hardware/Hardware.h"
#include <Arduino.h>
#include <driver/i2s.h>
#include <es8311.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <cmath>
#include <atomic>

namespace {
constexpr uint32_t kSampleRate = 48000;
constexpr size_t kFrames = 256;
enum class Kind : uint8_t { Mode, On, Off, Beat };
struct Message { Kind kind; uint8_t value; uint8_t velocity; };
StaticQueue_t queueControl;
uint8_t queueStorage[64 * sizeof(Message)];
QueueHandle_t queue = nullptr;
StaticTask_t taskControl;
StackType_t taskStack[2048];
int16_t pcm[kFrames * 2];
int16_t sineTable[256];
volatile uint32_t drops = 0;
volatile bool initialized = false;
audio::Mode currentMode = audio::Mode::Off;
std::atomic<uint8_t> requestedMode{0};
uint8_t currentNote = 0;
uint32_t phase = 0, phaseStep = 0;
uint32_t clickFrames = 0;
uint32_t clickPhaseStep = 0;
uint8_t noteVelocity = 0;

bool post(Message message) {
    if (queue && xQueueSend(queue, &message, 0) == pdTRUE) return true;
    ++drops;
    return false;
}

void run(void*) {
    bool i2sRunning = true;
    for (;;) {
        Message message;
        if (currentMode == audio::Mode::Off) {
            if (i2sRunning) {
                i2s_stop(I2S_NUM_0);
                i2sRunning = false;
            }
            xQueueReceive(queue, &message, portMAX_DELAY);
            if (message.kind == Kind::Mode) currentMode = static_cast<audio::Mode>(message.value);
            if (currentMode == audio::Mode::Off) continue;
            i2s_start(I2S_NUM_0);
            i2sRunning = true;
        }
        for (unsigned i = 0; i < 16 && xQueueReceive(queue, &message, 0) == pdTRUE; ++i) {
            switch (message.kind) {
            case Kind::Mode:
                currentMode = static_cast<audio::Mode>(message.value);
                noteVelocity = 0;
                clickFrames = 0;
                break;
            case Kind::On:
                if (currentMode == audio::Mode::Notes) {
                    currentNote = message.value;
                    noteVelocity = message.velocity;
                    phase = 0;
                    const double hz = 440.0 * std::pow(2.0, (static_cast<int>(currentNote) - 69) / 12.0);
                    phaseStep = static_cast<uint32_t>(hz * (4294967296.0 / kSampleRate));
                }
                break;
            case Kind::Off:
                if (message.value == currentNote) noteVelocity = 0;
                break;
            case Kind::Beat:
                if (currentMode == audio::Mode::Metronome) {
                    clickFrames = kSampleRate / 30;  // About 33 ms.
                    phase = 0;
                    clickPhaseStep = static_cast<uint32_t>(
                        (message.value ? 1200.0 : 800.0) * (4294967296.0 / kSampleRate));
                }
                break;
            }
        }
        for (size_t i = 0; i < kFrames; ++i) {
            int32_t sample = 0;
            if (currentMode == audio::Mode::Notes && noteVelocity) {
                sample = sineTable[phase >> 24] * noteVelocity / 127;
                phase += phaseStep;
            } else if (currentMode == audio::Mode::Metronome && clickFrames) {
                sample = sineTable[phase >> 24] * static_cast<int32_t>(clickFrames) /
                    (kSampleRate / 30);
                phase += clickPhaseStep;
                --clickFrames;
            }
            pcm[2 * i] = static_cast<int16_t>(sample);
            pcm[2 * i + 1] = static_cast<int16_t>(sample);
        }
        size_t written = 0;
        i2s_write(I2S_NUM_0, pcm, sizeof(pcm), &written, pdMS_TO_TICKS(20));
    }
}
}  // namespace

namespace audio {
bool begin(Mode initial) {
    for (unsigned i = 0; i < 256; ++i) {
        sineTable[i] = static_cast<int16_t>(2800 * std::sin(2.0 * 3.141592653589793 * i / 256));
    }
    i2s_config_t config{};
    config.mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_TX);
    config.sample_rate = kSampleRate;
    config.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
    config.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
    config.communication_format = I2S_COMM_FORMAT_STAND_I2S;
    config.intr_alloc_flags = 0;
    config.dma_buf_count = 4;
    config.dma_buf_len = kFrames;
    config.use_apll = false;
    config.tx_desc_auto_clear = true;
    config.fixed_mclk = 0;
    if (i2s_driver_install(I2S_NUM_0, &config, 0, nullptr) != ESP_OK) return false;
    const i2s_pin_config_t pins = {
        board::kAudioMclk, board::kAudioBclk, board::kAudioLrck,
        board::kAudioDataOut, I2S_PIN_NO_CHANGE
    };
    if (i2s_set_pin(I2S_NUM_0, &pins) != ESP_OK) return false;
    es8311_handle_t codec = es8311_create(I2C_NUM_0, ES8311_ADDRESS_0);
    if (!codec) return false;
    const es8311_clock_config_t clock = {
        false, false, true, kSampleRate * 256, kSampleRate
    };
    if (es8311_init(codec, &clock, ES8311_RESOLUTION_16, ES8311_RESOLUTION_16) != ESP_OK ||
        es8311_voice_volume_set(codec, 35, nullptr) != ESP_OK ||
        es8311_microphone_config(codec, false) != ESP_OK) return false;
    if (!hardware::setSpeakerAmplifier(initial != Mode::Off)) return false;
    currentMode = initial;
    requestedMode.store(static_cast<uint8_t>(initial), std::memory_order_relaxed);
    queue = xQueueCreateStatic(64, sizeof(Message), queueStorage, &queueControl);
    if (!queue) return false;
    initialized = xTaskCreateStaticPinnedToCore(
        run, "speaker", sizeof(taskStack), nullptr, 2, taskStack, &taskControl, 1) != nullptr;
    return initialized;
}

bool setMode(Mode mode) {
    if (!initialized || mode > Mode::Notes) return false;
    // Expander access stays on the UI task; audio task owns only I2S.
    if (!post({Kind::Mode, static_cast<uint8_t>(mode), 0})) return false;
    if (!hardware::setSpeakerAmplifier(mode != Mode::Off)) return false;
    requestedMode.store(static_cast<uint8_t>(mode), std::memory_order_relaxed);
    return true;
}
void noteOn(uint8_t note, uint8_t velocity) {
    if (requestedMode.load(std::memory_order_relaxed) == static_cast<uint8_t>(Mode::Notes))
        post({Kind::On, note, velocity});
}
void noteOff(uint8_t note) {
    if (requestedMode.load(std::memory_order_relaxed) == static_cast<uint8_t>(Mode::Notes))
        post({Kind::Off, note, 0});
}
void beat(bool accent) {
    if (requestedMode.load(std::memory_order_relaxed) == static_cast<uint8_t>(Mode::Metronome))
        post({Kind::Beat, static_cast<uint8_t>(accent), 0});
}
bool ready() { return initialized; }
uint32_t droppedEvents() { return drops; }
}  // namespace audio
