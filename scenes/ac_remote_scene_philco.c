#include "../ac_remote_app_i.h"
#include "../philco_ac_protocol.h"

typedef enum {
    button_power,
    button_mode,
    button_temp_up,
    button_fan,
    button_temp_down,
    button_swing,
    button_display,
    label_temperature,
} button_id;

typedef enum {
    command_swing,
    command_display_toggle,
} command_id;

// Power icon table: index 0=OFF state (show ON button), 1=ON state (show OFF button)
const Icon* power_icons[2][2] = {
    [0] = {&I_on_19x20, &I_on_hover_19x20},
    [1] = {&I_power_off_19x20, &I_power_off_hover_19x20},
};

// Mode icon table: Cool, Heat, Dry, Fan
const Icon* mode_icons[4][2] = {
    [0] = {&I_cold_19x20, &I_cold_hover_19x20},
    [1] = {&I_heat_19x20, &I_heat_hover_19x20},
    [2] = {&I_mode_dry_19x20, &I_mode_dry_hover_19x20},
    [3] = {&I_fan_19x20, &I_fan_hover_19x20},
};

// Fan icon table: Auto, Low, Med, High
const Icon* fan_icons[4][2] = {
    [0] = {&I_fan_speed_auto_19x20, &I_fan_speed_auto_hover_19x20},
    [1] = {&I_fan_speed_1_19x20, &I_fan_speed_1_hover_19x20},
    [2] = {&I_fan_speed_2_19x20, &I_fan_speed_2_hover_19x20},
    [3] = {&I_fan_speed_3_19x20, &I_fan_speed_3_hover_19x20},
};

// Mode to protocol value mapping
static const uint8_t mode_values[] = {
    PHILCO_MODE_COOL,
    PHILCO_MODE_HEAT,
    PHILCO_MODE_DRY,
    PHILCO_MODE_FAN_ONLY,
};

// Fan to protocol value mapping
static const uint8_t fan_values[] = {
    PHILCO_FAN_AUTO,
    PHILCO_FAN_LOW,
    PHILCO_FAN_MEDIUM,
    PHILCO_FAN_HIGH,
};

static char temp_buffer[4] = {0};

// ── Settings persistence ────────────────────────────────────────────

static bool ac_remote_load_settings(ACRemoteAppSettings* app_state) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_buffered_file_alloc(storage);
    FuriString* header = furi_string_alloc();

    uint32_t version = 0;
    bool success = false;
    do {
        if(!flipper_format_buffered_file_open_existing(ff, AC_REMOTE_APP_SETTINGS)) break;
        if(!flipper_format_read_header(ff, header, &version)) break;
        if(!furi_string_equal(header, "Philco AC") || (version != 1)) break;
        if(!flipper_format_read_uint32(ff, "Power", &app_state->power, 1)) break;
        if(!flipper_format_read_uint32(ff, "Mode", &app_state->mode, 1)) break;
        if(app_state->mode > 3) break;
        if(!flipper_format_read_uint32(ff, "Temperature", &app_state->temperature, 1)) break;
        if(app_state->temperature < PHILCO_TEMP_MIN || app_state->temperature > PHILCO_TEMP_MAX)
            break;
        if(!flipper_format_read_uint32(ff, "Fan", &app_state->fan, 1)) break;
        if(app_state->fan > 3) break;
        success = true;
    } while(false);

    furi_record_close(RECORD_STORAGE);
    furi_string_free(header);
    flipper_format_free(ff);
    return success;
}

static bool ac_remote_store_settings(ACRemoteAppSettings* app_state) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    FlipperFormat* ff = flipper_format_file_alloc(storage);

    bool success = false;
    do {
        if(!flipper_format_file_open_always(ff, AC_REMOTE_APP_SETTINGS)) break;
        if(!flipper_format_write_header_cstr(ff, "Philco AC", 1)) break;
        if(!flipper_format_write_comment_cstr(ff, "")) break;
        if(!flipper_format_write_uint32(ff, "Power", &app_state->power, 1)) break;
        if(!flipper_format_write_uint32(ff, "Mode", &app_state->mode, 1)) break;
        if(!flipper_format_write_uint32(ff, "Temperature", &app_state->temperature, 1)) break;
        if(!flipper_format_write_uint32(ff, "Fan", &app_state->fan, 1)) break;
        success = true;
    } while(false);

    furi_record_close(RECORD_STORAGE);
    flipper_format_free(ff);
    return success;
}

// ── IR transmission ─────────────────────────────────────────────────

static void ac_remote_send_settings(const ACRemoteAppSettings* settings) {
    uint8_t packet[PHILCO_PACKET_SIZE];

    philco_build_packet(
        packet,
        settings->temperature,
        mode_values[settings->mode],
        fan_values[settings->fan],
        !settings->power, // power_off flag
        false);

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

static void ac_remote_send_swing(const ACRemoteAppSettings* settings) {
    uint8_t packet[PHILCO_PACKET_SIZE];

    philco_build_packet(
        packet,
        settings->temperature,
        mode_values[settings->mode],
        fan_values[settings->fan],
        false,
        true); // swing toggle

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

static void ac_remote_send_display_toggle(const ACRemoteAppSettings* settings) {
    uint8_t packet[PHILCO_PACKET_SIZE];

    philco_build_display_toggle_packet(
        packet,
        settings->temperature,
        mode_values[settings->mode],
        fan_values[settings->fan]);

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

// ── Button callbacks ────────────────────────────────────────────────

static void ac_remote_button_callback(void* context, uint32_t index) {
    AC_RemoteApp* app = context;
    uint32_t event = ac_remote_custom_event_pack(AC_RemoteCustomEventTypeButtonPressed, index);
    view_dispatcher_send_custom_event(app->view_dispatcher, event);
}

// ── Scene handlers ──────────────────────────────────────────────────

void ac_remote_scene_philco_on_enter(void* context) {
    AC_RemoteApp* app = context;
    ACRemotePanel* panel = app->ac_remote_panel;

    if(!ac_remote_load_settings(&app->app_state)) {
        app->app_state.power = 1;
        app->app_state.mode = 0; // Cool
        app->app_state.fan = 0;  // Auto
        app->app_state.temperature = 24;
        app->app_state.swing_on = 0;
    }

    view_stack_add_view(app->view_stack, ac_remote_panel_get_view(panel));
    ac_remote_panel_reserve(panel, 2, 4);

    // Row 0: Power + Mode
    ac_remote_panel_add_item(
        panel, button_power, 0, 0, 6, 17,
        power_icons[app->app_state.power][0],
        power_icons[app->app_state.power][1],
        ac_remote_button_callback, NULL, context);
    ac_remote_panel_add_icon(panel, 5, 39, &I_power_text_21x5);

    ac_remote_panel_add_item(
        panel, button_mode, 1, 0, 39, 17,
        mode_icons[app->app_state.mode][0],
        mode_icons[app->app_state.mode][1],
        ac_remote_button_callback, NULL, context);
    ac_remote_panel_add_icon(panel, 40, 39, &I_mode_text_17x5);

    // Temperature frame
    ac_remote_panel_add_icon(panel, 0, 59, &I_frame_30x39);

    // Row 1: Temp Up + Fan
    ac_remote_panel_add_item(
        panel, button_temp_up, 0, 1, 3, 47,
        &I_tempup_24x21, &I_tempup_hover_24x21,
        ac_remote_button_callback, NULL, context);

    ac_remote_panel_add_item(
        panel, button_fan, 1, 1, 39, 50,
        fan_icons[app->app_state.fan][0],
        fan_icons[app->app_state.fan][1],
        ac_remote_button_callback, NULL, context);
    ac_remote_panel_add_icon(panel, 43, 72, &I_fan_text_12x5);

    // Row 2: Temp Down + Swing
    ac_remote_panel_add_item(
        panel, button_temp_down, 0, 2, 3, 89,
        &I_tempdown_24x21, &I_tempdown_hover_24x21,
        ac_remote_button_callback, NULL, context);

    ac_remote_panel_add_item(
        panel, button_swing, 1, 2, 39, 83,
        &I_swing_19x20, &I_swing_hover_19x20,
        ac_remote_button_callback, NULL, context);
    ac_remote_panel_add_icon(panel, 38, 105, &I_swing_text_20x5);

    // Row 3: Display toggle (uses LED icon)
    ac_remote_panel_add_item(
        panel, button_display, 0, 3, 1, 115,
        &I_led_19x11, &I_led_hover_19x11,
        ac_remote_button_callback, NULL, context);

    // Title label
    ac_remote_panel_add_label(panel, 0, 6, 11, FontPrimary, "Philco AC");

    // Temperature value label
    snprintf(temp_buffer, sizeof(temp_buffer), "%ld", app->app_state.temperature);
    ac_remote_panel_add_label(panel, label_temperature, 4, 82, FontKeyboard, temp_buffer);

    view_set_orientation(view_stack_get_view(app->view_stack), ViewOrientationVertical);
    view_dispatcher_switch_to_view(app->view_dispatcher, AC_RemoteAppViewStack);
}

bool ac_remote_scene_philco_on_event(void* context, SceneManagerEvent event) {
    AC_RemoteApp* app = context;
    ACRemotePanel* panel = app->ac_remote_panel;

    if(event.type != SceneManagerEventTypeCustom) {
        return false;
    }

    uint16_t event_type;
    int16_t event_value;
    ac_remote_custom_event_unpack(event.event, &event_type, &event_value);

    // Handle send events
    if(event_type == AC_RemoteCustomEventTypeSendSettings) {
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_blink_white_100);
        ac_remote_send_settings(&app->app_state);
        notification_message(notifications, &sequence_blink_stop);
        furi_record_close(RECORD_NOTIFICATION);
        return true;
    }

    if(event_type == AC_RemoteCustomEventTypeSendCommand) {
        NotificationApp* notifications = furi_record_open(RECORD_NOTIFICATION);
        notification_message(notifications, &sequence_blink_white_100);
        if(event_value == command_swing) {
            ac_remote_send_swing(&app->app_state);
        } else if(event_value == command_display_toggle) {
            ac_remote_send_display_toggle(&app->app_state);
        }
        notification_message(notifications, &sequence_blink_stop);
        furi_record_close(RECORD_NOTIFICATION);
        return true;
    }

    // Handle button presses
    if(event_type != AC_RemoteCustomEventTypeButtonPressed) {
        return false;
    }

    switch(event_value) {
    case button_power:
        app->app_state.power = app->app_state.power ? 0 : 1;
        ac_remote_panel_item_set_icons(
            panel, button_power,
            power_icons[app->app_state.power][0],
            power_icons[app->app_state.power][1]);
        break;

    case button_mode:
        app->app_state.mode = (app->app_state.mode + 1) % 4;
        ac_remote_panel_item_set_icons(
            panel, button_mode,
            mode_icons[app->app_state.mode][0],
            mode_icons[app->app_state.mode][1]);
        if(!app->app_state.power) return true;
        break;

    case button_temp_up:
        if(app->app_state.temperature < PHILCO_TEMP_MAX) {
            app->app_state.temperature++;
            snprintf(temp_buffer, sizeof(temp_buffer), "%ld", app->app_state.temperature);
            ac_remote_panel_label_set_string(panel, label_temperature, temp_buffer);
        }
        if(!app->app_state.power) return true;
        break;

    case button_temp_down:
        if(app->app_state.temperature > PHILCO_TEMP_MIN) {
            app->app_state.temperature--;
            snprintf(temp_buffer, sizeof(temp_buffer), "%ld", app->app_state.temperature);
            ac_remote_panel_label_set_string(panel, label_temperature, temp_buffer);
        }
        if(!app->app_state.power) return true;
        break;

    case button_fan:
        app->app_state.fan = (app->app_state.fan + 1) % 4;
        ac_remote_panel_item_set_icons(
            panel, button_fan,
            fan_icons[app->app_state.fan][0],
            fan_icons[app->app_state.fan][1]);
        if(!app->app_state.power) return true;
        break;

    case button_swing:
        if(!app->app_state.power) return true;
        app->app_state.swing_on = app->app_state.swing_on ? 0 : 1;
        view_dispatcher_send_custom_event(
            app->view_dispatcher,
            ac_remote_custom_event_pack(AC_RemoteCustomEventTypeSendCommand, command_swing));
        return true;

    case button_display:
        if(!app->app_state.power) return true;
        view_dispatcher_send_custom_event(
            app->view_dispatcher,
            ac_remote_custom_event_pack(
                AC_RemoteCustomEventTypeSendCommand, command_display_toggle));
        return true;

    default:
        return false;
    }

    // Send settings for non-command buttons
    view_dispatcher_send_custom_event(
        app->view_dispatcher,
        ac_remote_custom_event_pack(AC_RemoteCustomEventTypeSendSettings, 0));
    return true;
}

void ac_remote_scene_philco_on_exit(void* context) {
    AC_RemoteApp* app = context;
    ac_remote_store_settings(&app->app_state);
    view_stack_remove_view(app->view_stack, ac_remote_panel_get_view(app->ac_remote_panel));
    ac_remote_panel_reset(app->ac_remote_panel);
}
