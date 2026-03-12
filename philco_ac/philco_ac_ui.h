#pragma once

#include <gui/gui.h>
#include <furi.h>
#include "philco_ac_storage.h"

typedef struct PhilcoApp {
    PhilcoSettings settings;
    bool sending;
    FuriMutex* mutex;
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
} PhilcoApp;

void philco_draw_callback(Canvas* canvas, void* ctx);
