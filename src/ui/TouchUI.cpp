#include "TouchUI.h"
#include <lvgl.h>
#include <cstdint>

namespace {
lv_obj_t* steps[16];
lv_obj_t* selection;
lv_obj_t* playText;
bool playing = false;

lv_obj_t* label(lv_obj_t* parent, const char* text, int x, int y) {
    auto* object = lv_label_create(parent);
    lv_label_set_text(object, text);
    lv_obj_set_pos(object, x, y);
    return object;
}

void selectStep(lv_event_t* event) {
    auto* target = lv_event_get_target(event);
    for (unsigned i = 0; i < 16; ++i) {
        if (steps[i] == target) {
            lv_obj_add_state(steps[i], LV_STATE_CHECKED);
            lv_label_set_text_fmt(selection, "Step %02u selected", i + 1);
        } else {
            lv_obj_clear_state(steps[i], LV_STATE_CHECKED);
        }
    }
}

void togglePlay(lv_event_t*) {
    playing = !playing;
    lv_label_set_text(playText, playing ? "Stop" : "Play");
}
}  // namespace

namespace ui {
void create(bool touchAvailable) {
    auto* screen = lv_scr_act();
    lv_obj_clear_flag(screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(screen, lv_color_hex(0x151c28), 0);
    lv_obj_set_style_text_color(screen, lv_color_hex(0xf1f5fa), 0);
    auto* title = label(screen, "MIDI Sequencer", 12, 12);
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    label(screen, "Pattern 01", 12, 47);
    label(screen, "120 BPM", 190, 47);
    auto* play = lv_btn_create(screen);
    lv_obj_set_pos(play, 350, 12);
    lv_obj_set_size(play, 118, 52);
    playText = lv_label_create(play);
    lv_label_set_text(playText, "Play");
    lv_obj_center(playText);
    lv_obj_add_event_cb(play, togglePlay, LV_EVENT_CLICKED, nullptr);

    for (unsigned i = 0; i < 16; ++i) {
        steps[i] = lv_btn_create(screen);
        lv_obj_set_pos(steps[i], 12 + (i % 8) * 58, 86 + (i / 8) * 66);
        lv_obj_set_size(steps[i], 50, 58);
        lv_obj_set_style_bg_color(steps[i], lv_color_hex(0x344158), 0);
        lv_obj_set_style_bg_color(steps[i], lv_color_hex(0x007e91), LV_STATE_CHECKED);
        lv_obj_set_style_border_width(steps[i], 3, LV_STATE_CHECKED);
        lv_obj_set_style_border_color(steps[i], lv_color_hex(0x72eddf), LV_STATE_CHECKED);
        auto* number = lv_label_create(steps[i]);
        lv_label_set_text_fmt(number, "%02u", i + 1);
        lv_obj_center(number);
        lv_obj_add_event_cb(steps[i], selectStep, LV_EVENT_CLICKED, nullptr);
    }
    lv_obj_add_state(steps[0], LV_STATE_CHECKED);
    selection = label(screen, "Step 01 selected", 12, 228);
    label(screen, touchAvailable ? "Touch ready - tap a step" : "ERROR: FT6336 touch not detected", 12, 258);
    label(screen, "UI test only | MIDI off | Play has no timing", 12, 292);
}
}  // namespace ui
