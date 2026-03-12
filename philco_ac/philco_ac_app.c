#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>

#include "philco_ac_protocol.h"
#include "philco_ac_ui.h"
#include "philco_ac_storage.h"

// Mode value mapping (indexed by mode_index 0-4: Cool, Heat, Dry, Fan, Off)
static const uint8_t mode_values[] = {
    PHILCO_MODE_COOL,
    PHILCO_MODE_HEAT,
    PHILCO_MODE_DRY,
    PHILCO_MODE_FAN_ONLY,
    0x00, // OFF (uses COOL mode with power_off flag)
};

// Fan value mapping (indexed by fan_index 0-3: Auto, Low, Med, High)
static const uint8_t fan_values[] = {
    PHILCO_FAN_AUTO,
    PHILCO_FAN_LOW,
    PHILCO_FAN_MEDIUM,
    PHILCO_FAN_HIGH,
};

static void philco_input_callback(InputEvent* event, void* ctx) {
    PhilcoApp* app = ctx;
    furi_message_queue_put(app->event_queue, event, FuriWaitForever);
}

static void philco_send_command(PhilcoApp* app, bool swing_toggle) {
    bool is_off = (app->settings.mode_index == 4);
    uint8_t packet[PHILCO_PACKET_SIZE];

    philco_build_packet(
        packet,
        app->settings.temperature,
        is_off ? PHILCO_MODE_COOL : mode_values[app->settings.mode_index],
        fan_values[app->settings.fan_index],
        is_off,
        swing_toggle);

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

static void philco_send_display_toggle(PhilcoApp* app) {
    bool is_off = (app->settings.mode_index == 4);
    uint8_t packet[PHILCO_PACKET_SIZE];

    philco_build_display_toggle_packet(
        packet,
        app->settings.temperature,
        is_off ? PHILCO_MODE_COOL : mode_values[app->settings.mode_index],
        fan_values[app->settings.fan_index]);

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

int32_t philco_ac_app(void* p) {
    UNUSED(p);

    PhilcoApp* app = malloc(sizeof(PhilcoApp));
    app->sending = false;
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    philco_load_settings(app);

    // GUI setup with vertical orientation
    app->view_port = view_port_alloc();
    view_port_set_orientation(app->view_port, ViewPortOrientationVertical);
    view_port_draw_callback_set(app->view_port, philco_draw_callback, app);
    view_port_input_callback_set(app->view_port, philco_input_callback, app);
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    // Main event loop
    InputEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(app->event_queue, &event, 100) == FuriStatusOk) {
            furi_mutex_acquire(app->mutex, FuriWaitForever);

            if(event.type == InputTypeShort) {
                switch(event.key) {
                case InputKeyUp:
                    if(app->settings.temperature < PHILCO_TEMP_MAX)
                        app->settings.temperature++;
                    break;
                case InputKeyDown:
                    if(app->settings.temperature > PHILCO_TEMP_MIN)
                        app->settings.temperature--;
                    break;
                case InputKeyLeft:
                    // Mode prev
                    app->settings.mode_index =
                        (app->settings.mode_index + 4) % 5;
                    break;
                case InputKeyRight:
                    // Mode next
                    app->settings.mode_index =
                        (app->settings.mode_index + 1) % 5;
                    break;
                case InputKeyOk:
                    // Send IR command
                    app->sending = true;
                    furi_mutex_release(app->mutex);
                    view_port_update(app->view_port);
                    furi_delay_ms(10);
                    philco_send_command(app, false);
                    furi_mutex_acquire(app->mutex, FuriWaitForever);
                    break;
                case InputKeyBack:
                    running = false;
                    break;
                default:
                    break;
                }
            } else if(event.type == InputTypeLong) {
                switch(event.key) {
                case InputKeyUp:
                    // Display toggle
                    app->sending = true;
                    furi_mutex_release(app->mutex);
                    view_port_update(app->view_port);
                    furi_delay_ms(10);
                    philco_send_display_toggle(app);
                    furi_mutex_acquire(app->mutex, FuriWaitForever);
                    break;
                case InputKeyOk:
                    // Toggle swing
                    app->settings.swing_on = !app->settings.swing_on;
                    app->sending = true;
                    furi_mutex_release(app->mutex);
                    view_port_update(app->view_port);
                    furi_delay_ms(10);
                    philco_send_command(app, true);
                    furi_mutex_acquire(app->mutex, FuriWaitForever);
                    break;
                case InputKeyLeft:
                    // Fan speed prev
                    app->settings.fan_index =
                        (app->settings.fan_index + 3) % 4;
                    break;
                case InputKeyRight:
                    // Fan speed next
                    app->settings.fan_index =
                        (app->settings.fan_index + 1) % 4;
                    break;
                case InputKeyBack:
                    running = false;
                    break;
                default:
                    break;
                }
            } else if(event.type == InputTypeRelease) {
                if(app->sending) {
                    app->sending = false;
                }
            }

            furi_mutex_release(app->mutex);
            view_port_update(app->view_port);
        }
    }

    // Save settings and cleanup
    philco_save_settings(app);
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);
    free(app);

    return 0;
}
