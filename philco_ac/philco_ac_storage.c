#include "philco_ac_storage.h"
#include "philco_ac_ui.h"
#include "philco_ac_protocol.h"
#include <storage/storage.h>
#include <furi.h>

#define SETTINGS_PATH APP_DATA_PATH("settings.dat")

static void philco_settings_set_defaults(PhilcoSettings* settings) {
    settings->temperature = 24;
    settings->mode_index = 0; // Cool
    settings->fan_index = 0;  // Auto
    settings->swing_on = false;
}

static bool philco_settings_validate(const PhilcoSettings* settings) {
    if(settings->temperature < PHILCO_TEMP_MIN || settings->temperature > PHILCO_TEMP_MAX)
        return false;
    if(settings->mode_index > 4) return false;
    if(settings->fan_index > 3) return false;
    return true;
}

void philco_load_settings(PhilcoApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    bool loaded = false;
    if(storage_file_open(file, SETTINGS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        if(storage_file_read(file, &app->settings, sizeof(PhilcoSettings)) ==
           sizeof(PhilcoSettings)) {
            if(philco_settings_validate(&app->settings)) {
                loaded = true;
            }
        }
    }

    if(!loaded) {
        philco_settings_set_defaults(&app->settings);
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

void philco_save_settings(PhilcoApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &app->settings, sizeof(PhilcoSettings));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}
