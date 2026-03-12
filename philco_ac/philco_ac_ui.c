#include "philco_ac_ui.h"
#include "philco_ac_protocol.h"
#include <gui/elements.h>
#include <assets_icons.h>
#include "philco_ac_icons.h"

// Mode display data
static const char* mode_names[] = {"COOL", "HEAT", "DRY", "FAN", "OFF"};

// Fan display data
static const char* fan_names[] = {"AUTO", "LOW", "MED", "HIGH"};

// Mode icons table (indexed by mode_index 0-4)
static const Icon* mode_icons[] = {
    &I_mode_cool_12x12,
    &I_mode_heat_12x12,
    &I_mode_dry_12x12,
    &I_mode_fan_12x12,
    &I_mode_off_12x12,
};

// Fan speed icons table (indexed by fan_index 0-3)
static const Icon* fan_icons[] = {
    &I_fan_auto_20x8,
    &I_fan_low_20x8,
    &I_fan_med_20x8,
    &I_fan_high_20x8,
};

void philco_draw_callback(Canvas* canvas, void* ctx) {
    PhilcoApp* app = ctx;
    furi_mutex_acquire(app->mutex, FuriWaitForever);

    canvas_clear(canvas);
    bool is_off = (app->settings.mode_index == 4);

    // ── Zone 1: Header Bar (Y 0-13) ──
    if(is_off) {
        // Inverted header for OFF state
        canvas_draw_box(canvas, 0, 0, 64, 14);
        canvas_set_color(canvas, ColorWhite);
        canvas_draw_icon(canvas, 2, 1, mode_icons[app->settings.mode_index]);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 16, 11, "OFF");
        canvas_set_color(canvas, ColorBlack);
    } else {
        canvas_draw_icon(canvas, 2, 1, mode_icons[app->settings.mode_index]);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 16, 11, mode_names[app->settings.mode_index]);
        // Swing icon
        if(app->settings.swing_on) {
            canvas_draw_icon(canvas, 54, 3, &I_swing_on_8x8);
        } else {
            canvas_draw_icon(canvas, 54, 3, &I_swing_off_8x8);
        }
    }
    // Header separator
    canvas_draw_line(canvas, 0, 14, 63, 14);

    // ── Zone 2: Temperature Display (Y 16-55) ──
    if(is_off) {
        // Lighter frame for OFF state
        canvas_draw_rframe(canvas, 4, 18, 56, 36, 3);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 32, 36, AlignCenter, AlignCenter, "- -");
    } else {
        // Bold rounded frame around temperature
        elements_bold_rounded_frame(canvas, 4, 18, 56, 36);

        // Temperature digits in big font
        char temp_digits[4];
        snprintf(temp_digits, sizeof(temp_digits), "%d", app->settings.temperature);
        canvas_set_font(canvas, FontBigNumbers);
        // Measure digit width to position °C suffix
        uint16_t digits_w = canvas_string_width(canvas, temp_digits);
        // Center the combined "24°C" display
        uint16_t total_w = digits_w + 12; // ~12px for "°C" in FontPrimary
        int16_t start_x = 32 - (total_w / 2);
        canvas_draw_str(canvas, start_x, 28, temp_digits);

        // Draw °C suffix in FontPrimary at the right of digits
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, start_x + digits_w + 1, 38, "\xB0""C");
    }

    // ── Zone 3: Temperature Bar (Y 56-69) ──
    if(!is_off) {
        float progress =
            (float)(app->settings.temperature - PHILCO_TEMP_MIN) /
            (float)(PHILCO_TEMP_MAX - PHILCO_TEMP_MIN);
        elements_progress_bar(canvas, 4, 58, 56, progress);

        // Range labels
        canvas_set_font(canvas, FontSecondary);
        canvas_draw_str(canvas, 4, 69, "18");
        canvas_draw_str_aligned(canvas, 60, 69, AlignRight, AlignBottom, "30");
    } else {
        // Empty bar for OFF state
        canvas_draw_rframe(canvas, 4, 58, 56, 5, 1);
    }

    // ── Zone 4: Fan Speed (Y 70-85) ──
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 82, "Fan:");
    if(!is_off) {
        canvas_draw_icon(canvas, 24, 74, fan_icons[app->settings.fan_index]);
        canvas_draw_str(canvas, 46, 82, fan_names[app->settings.fan_index]);
    } else {
        canvas_draw_str(canvas, 30, 82, "- -");
    }

    // ── Zone 5: Status/Sending Area (Y 86-107) ──
    // Dotted separator at Y=87
    for(int x = 2; x < 62; x += 3) {
        canvas_draw_dot(canvas, x, 87);
    }

    if(app->sending) {
        elements_bold_rounded_frame(canvas, 6, 90, 52, 16);
        canvas_draw_icon(canvas, 10, 93, &I_send_ir_10x10);
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str(canvas, 22, 102, "SENDING");
    }

    // ── Zone 6: Button Hints (Y 108-127) ──
    // Solid separator
    canvas_draw_line(canvas, 0, 109, 63, 109);

    canvas_set_font(canvas, FontSecondary);

    // Row 1: Up/Down Temp | Left/Right Mode
    canvas_draw_icon(canvas, 1, 111, &I_ButtonUp_7x4);
    canvas_draw_icon(canvas, 1, 115, &I_ButtonDown_7x4);
    canvas_draw_str(canvas, 10, 118, "Temp");

    canvas_draw_icon(canvas, 35, 112, &I_ButtonLeft_4x7);
    canvas_draw_icon(canvas, 40, 112, &I_ButtonRight_4x7);
    canvas_draw_str(canvas, 46, 118, "Mode");

    // Row 2: OK Send | Hold:More
    canvas_draw_icon(canvas, 1, 120, &I_ButtonCenter_7x7);
    canvas_draw_str(canvas, 10, 127, "Send");
    canvas_draw_str(canvas, 35, 127, "Hold:More");

    furi_mutex_release(app->mutex);
}
