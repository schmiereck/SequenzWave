#include "TouchUI.h"
#include "app/SequencerRuntime.h"
#include "hardware/Hardware.h"
#include "storage/SettingsStore.h"
#include <lvgl.h>
#include <cstdio>

namespace {
lv_obj_t* steps[16];
lv_obj_t* stepText[16];
lv_obj_t* selection;
lv_obj_t* playText;
lv_obj_t* bpmText;
lv_obj_t* restText;
lv_obj_t* footer;
lv_obj_t* settings;
lv_obj_t* gateText;
lv_obj_t* velocityText;
lv_obj_t* lightText;
storage::Data saved;
sequencer::Pattern& pattern = saved.pattern;
unsigned selected = 0;
uint16_t& bpm = saved.bpm;
int8_t shownStep = -2;
bool shownPlaying = false;
storage::SaveStatus shownSaveStatus = storage::SaveStatus::Error;
uint32_t refreshedAt = 0;

lv_obj_t* label(lv_obj_t* parent, const char* text, int x, int y) {
    auto* object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_pos(object, x, y);
    return object;
}

lv_obj_t* button(lv_obj_t* parent, const char* text, int x, int y, int width,
                 lv_event_cb_t callback, intptr_t action = 0) {
    auto* object = lv_btn_create(parent);
    lv_obj_set_pos(object, x, y);
    lv_obj_set_size(object, width, 44);
    lv_obj_set_style_pad_all(object, 2, 0);
    auto* caption = lv_label_create(object);
    lv_label_set_text(caption, text);
    lv_obj_center(caption);
    lv_obj_add_event_cb(object, callback, LV_EVENT_CLICKED, reinterpret_cast<void*>(action));
    return object;
}

void noteName(uint8_t note, char* text, size_t size) {
    const char* names[] = {"C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B"};
    snprintf(text, size, "%s%d", names[note % 12], static_cast<int>(note / 12) - 1);
}

void showEditor() {
    const auto& step = pattern.steps[selected];
    char name[8];
    noteName(step.note, name, sizeof(name));
    lv_label_set_text_fmt(selection, "Step %02u: %s  |  Gate %u%%", selected + 1, name, step.gate);
    lv_label_set_text(restText, step.enabled ? "Rest" : "Enable");
    for (unsigned i = 0; i < 16; ++i) {
        noteName(pattern.steps[i].note, name, sizeof(name));
        lv_label_set_text_fmt(stepText[i], "%02u\n%s", i + 1,
            pattern.steps[i].enabled ? name : "--");
        lv_obj_set_style_border_width(steps[i], i == selected ? 3 : 0, 0);
    }
    if (gateText) lv_label_set_text_fmt(gateText, "Gate %u%%", step.gate);
    if (velocityText) lv_label_set_text_fmt(velocityText, "Velocity %u", step.velocity);
}

bool submit(app::Command command) {
    const bool accepted = app::send(command);
    if (!accepted) lv_label_set_text(footer, "Busy - please repeat the edit");
    return accepted;
}

void selectStep(lv_event_t* event) {
    selected = static_cast<unsigned>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    showEditor();
}

void play(lv_event_t*) {
    app::Command command;
    command.action = app::Action::Toggle;
    submit(command);
}

void tempo(lv_event_t* event) {
    const int change = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    const int next = bpm + change;
    if (next < 30 || next > 300) return;
    app::Command command;
    command.action = app::Action::Tempo;
    command.value = static_cast<uint16_t>(next);
    if (submit(command)) {
        bpm = command.value;
        lv_label_set_text_fmt(bpmText, "%u BPM", bpm);
        storage::schedule(saved);
    }
}

void edit(lv_event_t* event) {
    const int action = static_cast<int>(reinterpret_cast<intptr_t>(lv_event_get_user_data(event)));
    auto next = pattern.steps[selected];
    if (action == 1000) next.enabled = !next.enabled;
    else if (action == 2000 || action == -2000) {
        const int value = next.gate + (action > 0 ? 5 : -5);
        if (value < 5 || value > 100) return;
        next.gate = static_cast<uint8_t>(value);
    } else if (action == 3000 || action == -3000) {
        const int value = next.velocity + (action > 0 ? 1 : -1);
        if (value < 1 || value > 127) return;
        next.velocity = static_cast<uint8_t>(value);
    } else {
        const int value = next.note + action;
        if (value < 0 || value > 127) return;
        next.note = static_cast<uint8_t>(value);
    }
    app::Command command;
    command.action = app::Action::Edit;
    command.value = selected;
    command.step = next;
    if (submit(command)) {
        pattern.steps[selected] = next;
        showEditor();
        storage::schedule(saved);
    }
}

void lightChanged(lv_event_t* event) {
    hardware::setBrightness(static_cast<uint8_t>(lv_slider_get_value(lv_event_get_target(event))));
    saved.brightness = hardware::brightness();
    storage::schedule(saved);
    lv_label_set_text_fmt(lightText, "Brightness %u%%", hardware::brightness());
}
void closeSettings(lv_event_t*) {
    storage::schedule(saved);
    storage::saveNow();
    lv_obj_add_flag(settings, LV_OBJ_FLAG_HIDDEN);
}
void saveSettings(lv_event_t*) {
    storage::schedule(saved);
    storage::saveNow();
}
void openSettings(lv_event_t*) {
    showEditor();
    lv_obj_clear_flag(settings, LV_OBJ_FLAG_HIDDEN);
}
}  // namespace

namespace ui {
void create(bool touchAvailable, const storage::Data& loaded) {
    saved = loaded;
    auto* screen = lv_scr_act();
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x151c28), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xf1f5fa), 0);
    label(screen, "MIDI Sequencer", 10, 5);
    bpmText = label(screen, "", 178, 30);
    lv_label_set_text_fmt(bpmText, "%u BPM", bpm);
    button(screen, "-", 120, 20, 44, tempo, -1);
    button(screen, "+", 258, 20, 44, tempo, 1);
    auto* transport = button(screen, "Play", 368, 12, 102, play);
    playText = lv_obj_get_child(transport, 0);

    for (unsigned i = 0; i < 16; ++i) {
        const int groupGap = (i % 8) >= 4 ? 10 : 0;
        steps[i] = button(screen, "", 10 + (i % 8) * 58 + groupGap,
                          76 + (i / 8) * 54, 50, selectStep, i);
        lv_obj_set_style_bg_color(steps[i], lv_color_hex(0x344158), 0);
        lv_obj_set_style_border_color(steps[i], lv_color_hex(0x72eddf), 0);
        stepText[i] = lv_obj_get_child(steps[i], 0);
    }
    selection = label(screen, "", 10, 187);
    button(screen, "Oct -", 10, 212, 82, edit, -12);
    button(screen, "Note -", 102, 212, 82, edit, -1);
    button(screen, "Note +", 194, 212, 82, edit, 1);
    button(screen, "Oct +", 286, 212, 82, edit, 12);
    auto* rest = button(screen, "Rest", 378, 212, 92, edit, 1000);
    restText = lv_obj_get_child(rest, 0);
    button(screen, "Settings", 10, 266, 100, openSettings);
    button(screen, "Save", 118, 266, 72, saveSettings);
    footer = label(screen, touchAvailable ? "Pattern 01 | Mock MIDI" : "ERROR: Touch not detected", 202, 280);

    // Modal settings page: generous finger targets; transport continues underneath.
    settings = lv_obj_create(screen);
    lv_obj_set_pos(settings, 0, 0);
    lv_obj_set_size(settings, 480, 320);
    lv_obj_set_style_pad_all(settings, 0, 0);
    lv_obj_set_style_border_width(settings, 0, 0);
    lv_obj_set_style_radius(settings, 0, 0);
    lv_obj_set_style_bg_color(settings, lv_color_hex(0x151c28), 0);
    lv_obj_set_style_text_color(settings, lv_color_hex(0xf1f5fa), 0);
    lv_obj_clear_flag(settings, LV_OBJ_FLAG_SCROLLABLE);
    label(settings, "Selected step / Display", 16, 14);
    button(settings, "Back", 368, 6, 96, closeSettings);
    gateText = label(settings, "", 16, 76);
    button(settings, "-", 220, 60, 80, edit, -2000);
    button(settings, "+", 320, 60, 80, edit, 2000);
    velocityText = label(settings, "", 16, 134);
    button(settings, "-", 220, 118, 80, edit, -3000);
    button(settings, "+", 320, 118, 80, edit, 3000);
    lightText = label(settings, "", 16, 190);
    lv_label_set_text_fmt(lightText, "Brightness %u%%", hardware::brightness());
    auto* slider = lv_slider_create(settings);
    lv_obj_set_pos(slider, 30, 235);
    lv_obj_set_size(slider, 420, 24);
    lv_obj_set_ext_click_area(slider, 12);
    lv_slider_set_range(slider, 2, 100);
    lv_slider_set_value(slider, hardware::brightness(), LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, lightChanged, LV_EVENT_VALUE_CHANGED, nullptr);
    label(settings, "Auto-save: 2 s idle | Back saves now", 16, 288);
    lv_obj_add_flag(settings, LV_OBJ_FLAG_HIDDEN);
    showEditor();
}

void refresh() {
    if (lv_tick_elaps(refreshedAt) < 30) return;
    refreshedAt = lv_tick_get();
    const auto state = app::snapshot();
    const auto save = storage::status();
    if (save != shownSaveStatus) {
        shownSaveStatus = save;
        const char* message = "Pattern 01 | Mock MIDI | Defaults";
        if (save == storage::SaveStatus::Pending) message = "Pattern 01 | Mock MIDI | Saving...";
        else if (save == storage::SaveStatus::Saved) message = "Pattern 01 | Mock MIDI | Saved";
        else if (save == storage::SaveStatus::Error) message = "SAVE ERROR | Check serial log";
        lv_label_set_text(footer, message);
    }
    if (state.transport.playing != shownPlaying) {
        shownPlaying = state.transport.playing;
        lv_label_set_text(playText, shownPlaying ? "Stop" : "Play");
    }
    if (state.transport.step != shownStep) {
        shownStep = state.transport.step;
        for (unsigned i = 0; i < 16; ++i) {
            lv_obj_set_style_bg_color(steps[i], lv_color_hex(
                static_cast<int>(i) == shownStep ? 0x986600 : 0x344158), 0);
        }
    }
}
}  // namespace ui
