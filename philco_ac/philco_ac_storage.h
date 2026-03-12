#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct __attribute__((packed)) {
    uint8_t temperature; // 18-30
    uint8_t mode_index;  // 0=Cool, 1=Heat, 2=Dry, 3=Fan, 4=Off
    uint8_t fan_index;   // 0=Auto, 1=Low, 2=Med, 3=High
    bool swing_on;
} PhilcoSettings;

// Forward declaration
typedef struct PhilcoApp PhilcoApp;

void philco_load_settings(PhilcoApp* app);
void philco_save_settings(PhilcoApp* app);
