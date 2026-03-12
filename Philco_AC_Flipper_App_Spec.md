# Philco AC Controller — Flipper Zero App Spec

## Overview

A Flipper Zero application (FAP) to control Philco air conditioners via infrared. The IR protocol was reverse-engineered from [esphome-philco-ac](https://github.com/brunoamui/esphome-philco-ac) (Philco PH9000QFM4). The app provides a visual interface for mode, temperature, fan speed, and swing control — far better than scrolling through a list of raw IR signals.

---

## 1. IR Protocol Specification

### Physical Layer

| Parameter        | Value   |
|------------------|---------|
| Carrier frequency | 38 kHz |
| Duty cycle        | 33%    |
| Encoding          | Pulse Distance, **LSB-first** |
| Total bits        | 120 (15 bytes) |

### Timing (microseconds)

| Element       | Mark  | Space |
|---------------|-------|-------|
| Header        | 8400  | 4200  |
| Bit `1`       | 600   | 1600  |
| Bit `0`       | 600   | 550   |
| Footer mark   | 600   | —     |
| Footer space  | 10000 | —     |

### Transmission order

```
[HEADER_MARK] [HEADER_SPACE]
  for each byte 0..14:
    for each bit 0..7 (LSB first):
      [BIT_MARK] [BIT_ONE_SPACE or BIT_ZERO_SPACE]
[FOOTER_MARK] [FOOTER_SPACE]
```

Total raw timings: 2 (header) + 240 (15 bytes × 8 bits × 2 mark/space) + 2 (footer) = **244 values**.

### Packet Structure (15 bytes)

```
Byte  Description                  Default/Formula
────  ───────────────────────────  ──────────────────────────
 0    Signature                    Always 0x56
 1    Temperature                  0x6E + (temp_celsius - 18)
 2    Reserved                     0x00
 3    Reserved                     0x00
 4    Mode (upper nibble) |        See tables below
      Fan  (lower nibble)
 5    Power / Swing flags          See below
 6    Reserved                     0x00
 7    Fixed                        0x0A
 8    Fixed                        0x0C
 9    Reserved                     0x00
10    Fixed                        0x0C
11    Reserved                     0x00
12    Fixed                        0x1E
13    Fixed                        0x11
14    Checksum                     Nibble sum of bytes 0-13
```

### Byte 1 — Temperature

Range: 18°C to 30°C

```
temp_celsius | byte value
-------------|----------
18           | 0x6E
19           | 0x6F
20           | 0x70
21           | 0x71
22           | 0x72
23           | 0x73
24           | 0x74
25           | 0x75
26           | 0x76
27           | 0x77
28           | 0x78
29           | 0x79
30           | 0x7A
```

Formula: `byte[1] = 0x6E + (temp - 18)`

### Byte 4 — Mode + Fan

**Upper nibble (mode):**

| Mode     | Value |
|----------|-------|
| HEAT     | 0x10  |
| COOL     | 0x20  |
| DRY      | 0x30  |
| FAN_ONLY | 0x40  |

**Lower nibble (fan speed):**

| Fan Speed | Value |
|-----------|-------|
| HIGH      | 0x00  |
| MEDIUM    | 0x01  |
| AUTO      | 0x02  |
| LOW       | 0x03  |

Example: COOL + AUTO = `0x20 | 0x02` = `0x22`

### Byte 5 — Power & Swing Flags

| State          | Value |
|----------------|-------|
| Power ON       | 0x00  |
| Power OFF      | 0xC0  |
| Swing toggle   | 0x02  |

- Power OFF sets `byte[5] = 0xC0` (bit 7 and bit 6 set).
- Swing is a **toggle** — sending `0x02` flips the current swing state on the AC unit. It is NOT an absolute on/off.
- When neither OFF nor swing toggle, `byte[5] = 0x00`.

### Byte 14 — Checksum

Sum all nibbles of bytes 0 through 13, take the lower 8 bits:

```c
uint16_t checksum = 0;
for (int i = 0; i < 14; i++) {
    checksum += (packet[i] & 0x0F) + ((packet[i] >> 4) & 0x0F);
}
packet[14] = checksum & 0xFF;
```

### Worked Example

**Cool, 24°C, Fan Auto, Power ON:**

```
Byte  0: 0x56  (signature)
Byte  1: 0x74  (0x6E + 6)
Byte  2: 0x00
Byte  3: 0x00
Byte  4: 0x22  (COOL 0x20 | AUTO 0x02)
Byte  5: 0x00  (power on, no swing)
Byte  6: 0x00
Byte  7: 0x0A
Byte  8: 0x0C
Byte  9: 0x00
Byte 10: 0x0C
Byte 11: 0x00
Byte 12: 0x1E
Byte 13: 0x11
Byte 14: checksum → nibbles: 5+6+7+4+0+0+0+0+2+2+0+0+0+0+0+A+0+C+0+0+0+C+0+0+1+E+1+1
         = 5+6+7+4+2+2+10+12+12+1+14+1+1 = 77 = 0x4D
```

---

## 2. App Architecture

### File Structure

```
philco_ac/
├── application.fam           # App manifest
├── philco_ac.png             # 10x10 icon (1-bit)
├── images/
│   ├── cool_10x10.png        # Mode icons (optional)
│   ├── heat_10x10.png
│   ├── fan_10x10.png
│   ├── dry_10x10.png
│   └── off_10x10.png
├── philco_ac_app.c           # Entry point + main loop
├── philco_ac_protocol.h      # Protocol constants + packet builder
├── philco_ac_protocol.c      # build_packet(), calc_checksum(), transmit()
├── philco_ac_ui.h            # UI rendering
├── philco_ac_ui.c            # draw callback, screen layout
└── philco_ac_storage.h/.c    # Save/load last settings (optional)
```

### application.fam

```python
App(
    appid="philco_ac",
    name="Philco AC",
    apptype=FlipperAppType.EXTERNAL,
    entry_point="philco_ac_app",
    stack_size=2 * 1024,
    fap_category="Infrared",
    fap_icon="philco_ac.png",
    fap_icon_assets="images",
    fap_author="YourName",
    fap_version="1.0",
    fap_description="Philco AC IR remote control",
)
```

---

## 3. App State

```c
typedef enum {
    PhilcoModeOff,
    PhilcoModeCool,
    PhilcoModeHeat,
    PhilcoModeDry,
    PhilcoModeFanOnly,
} PhilcoMode;

typedef enum {
    PhilcoFanAuto,
    PhilcoFanLow,
    PhilcoFanMedium,
    PhilcoFanHigh,
} PhilcoFanSpeed;

typedef struct {
    PhilcoMode mode;
    PhilcoFanSpeed fan;
    uint8_t temperature;      // 18-30
    bool swing_on;            // UI state (tracks toggle)
    bool transmitting;        // Brief flag for TX animation
} PhilcoAcState;
```

---

## 4. UI Design

### Screen Layout (128×64 pixels)

```
┌────────────────────────────┐
│  ❄ COOL              24°C │   ← Row 1: mode icon + name, temperature (large font)
│                            │
│  ══════════════════════    │   ← Row 2: temperature bar (18-30 range)
│                            │
│  Fan: ▓▓▓░  AUTO           │   ← Row 3: fan level visual + label
│  Swing: ON                 │   ← Row 4: swing status
│                            │
│  [OK] Send   [◄►] Temp    │   ← Row 5: button hints
└────────────────────────────┘
```

### Controls

| Button         | Action                                   |
|----------------|------------------------------------------|
| **Up**         | Cycle mode: Cool → Heat → Dry → Fan → Off |
| **Down**       | Cycle mode (reverse)                     |
| **Left**       | Temperature -1°C (min 18)               |
| **Right**      | Temperature +1°C (max 30)               |
| **OK (short)** | Transmit current settings to AC          |
| **OK (long)**  | Toggle swing on/off                      |
| **Back**       | Save settings & exit app                 |

### Visual Feedback

- On **OK press**: briefly flash/invert the screen or show "SENDING..." for ~300ms to confirm IR transmission.
- **Mode OFF**: grey out temperature and fan controls, show prominent "OFF" label.
- **Temperature bar**: filled proportionally from 18 (empty) to 30 (full). Current value in large bold font.

### Mode Display

| Mode     | Icon suggestion | Label    |
|----------|----------------|----------|
| Cool     | Snowflake      | COOL     |
| Heat     | Sun/Flame      | HEAT     |
| Dry      | Water drop     | DRY      |
| Fan Only | Fan blades     | FAN      |
| Off      | Power symbol   | OFF      |

---

## 5. Protocol Implementation (C)

### philco_ac_protocol.h

```c
#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <infrared.h>
#include <infrared_worker.h>

#define PHILCO_SIGNATURE     0x56
#define PHILCO_PACKET_SIZE   15
#define PHILCO_TOTAL_TIMINGS 244  // 2 + (15*8*2) + 2

#define PHILCO_HEADER_MARK   8400
#define PHILCO_HEADER_SPACE  4200
#define PHILCO_BIT_MARK      600
#define PHILCO_BIT_ONE_SPACE 1600
#define PHILCO_BIT_ZERO_SPACE 550
#define PHILCO_FOOTER_MARK   600
#define PHILCO_FOOTER_SPACE  10000

#define PHILCO_CARRIER_FREQ  38000
#define PHILCO_DUTY_CYCLE    0.33f

#define PHILCO_TEMP_MIN      18
#define PHILCO_TEMP_MAX      30

// Mode values (upper nibble of byte 4)
#define PHILCO_MODE_HEAT     0x10
#define PHILCO_MODE_COOL     0x20
#define PHILCO_MODE_DRY      0x30
#define PHILCO_MODE_FAN_ONLY 0x40

// Fan values (lower nibble of byte 4)
#define PHILCO_FAN_HIGH      0x00
#define PHILCO_FAN_MEDIUM    0x01
#define PHILCO_FAN_AUTO      0x02
#define PHILCO_FAN_LOW       0x03

// Build a 15-byte packet from the given state
void philco_build_packet(
    uint8_t* packet,        // out: 15-byte buffer
    uint8_t temperature,    // 18-30
    uint8_t mode,           // PHILCO_MODE_* constant
    uint8_t fan,            // PHILCO_FAN_* constant
    bool power_off,
    bool swing_toggle
);

// Convert 15-byte packet to raw IR timings array
// timings must point to a buffer of at least PHILCO_TOTAL_TIMINGS uint32_t values
void philco_packet_to_raw(
    const uint8_t* packet,
    uint32_t* timings,      // out: raw mark/space pairs
    size_t* timings_count   // out: actual count (244)
);

// Transmit using Flipper's IR subsystem
void philco_transmit(const uint32_t* timings, size_t count);
```

### philco_ac_protocol.c

```c
#include "philco_ac_protocol.h"

static uint8_t philco_calc_checksum(const uint8_t* packet) {
    uint16_t sum = 0;
    for(int i = 0; i < PHILCO_PACKET_SIZE - 1; i++) {
        sum += (packet[i] & 0x0F) + ((packet[i] >> 4) & 0x0F);
    }
    return sum & 0xFF;
}

void philco_build_packet(
    uint8_t* packet,
    uint8_t temperature,
    uint8_t mode,
    uint8_t fan,
    bool power_off,
    bool swing_toggle
) {
    // Zero out
    for(int i = 0; i < PHILCO_PACKET_SIZE; i++) packet[i] = 0;

    // Signature
    packet[0] = PHILCO_SIGNATURE;

    // Temperature
    uint8_t temp = temperature;
    if(temp < PHILCO_TEMP_MIN) temp = PHILCO_TEMP_MIN;
    if(temp > PHILCO_TEMP_MAX) temp = PHILCO_TEMP_MAX;
    packet[1] = 0x6E + (temp - PHILCO_TEMP_MIN);

    // Reserved
    packet[2] = 0x00;
    packet[3] = 0x00;

    // Mode + Fan
    packet[4] = mode | fan;

    // Power / Swing
    if(power_off) {
        packet[5] = 0xC0;
    } else if(swing_toggle) {
        packet[5] = 0x02;
    } else {
        packet[5] = 0x00;
    }

    // Fixed bytes
    packet[6]  = 0x00;
    packet[7]  = 0x0A;
    packet[8]  = 0x0C;
    packet[9]  = 0x00;
    packet[10] = 0x0C;
    packet[11] = 0x00;
    packet[12] = 0x1E;
    packet[13] = 0x11;

    // Checksum
    packet[14] = philco_calc_checksum(packet);
}

void philco_packet_to_raw(
    const uint8_t* packet,
    uint32_t* timings,
    size_t* timings_count
) {
    size_t idx = 0;

    // Header
    timings[idx++] = PHILCO_HEADER_MARK;
    timings[idx++] = PHILCO_HEADER_SPACE;

    // Data bits (LSB first)
    for(int byte_idx = 0; byte_idx < PHILCO_PACKET_SIZE; byte_idx++) {
        for(int bit = 0; bit < 8; bit++) {
            timings[idx++] = PHILCO_BIT_MARK;
            if((packet[byte_idx] >> bit) & 1) {
                timings[idx++] = PHILCO_BIT_ONE_SPACE;
            } else {
                timings[idx++] = PHILCO_BIT_ZERO_SPACE;
            }
        }
    }

    // Footer
    timings[idx++] = PHILCO_FOOTER_MARK;
    timings[idx++] = PHILCO_FOOTER_SPACE;

    *timings_count = idx;
}

void philco_transmit(const uint32_t* timings, size_t count) {
    // Flipper Zero raw IR transmission
    // Timings alternate: mark, space, mark, space...
    // First value is always a mark
    infrared_send_raw_ext(
        timings,
        count,
        true,            // start_from_mark
        PHILCO_CARRIER_FREQ,
        PHILCO_DUTY_CYCLE
    );
}
```

---

## 6. Main App Loop (Skeleton)

### philco_ac_app.c

```c
#include <furi.h>
#include <furi_hal.h>
#include <gui/gui.h>
#include <input/input.h>
#include <storage/storage.h>

#include "philco_ac_protocol.h"

#define SETTINGS_PATH APP_DATA_PATH("settings.dat")

typedef struct {
    uint8_t temperature;
    uint8_t mode_index;    // 0=Cool,1=Heat,2=Dry,3=Fan,4=Off
    uint8_t fan_index;     // 0=Auto,1=Low,2=Med,3=High
    bool swing_on;
} PhilcoSettings;

typedef struct {
    PhilcoSettings settings;
    bool sending;
    FuriMutex* mutex;
    Gui* gui;
    ViewPort* view_port;
    FuriMessageQueue* event_queue;
} PhilcoApp;

// Mode/fan mapping tables
static const uint8_t mode_values[] = {
    PHILCO_MODE_COOL, PHILCO_MODE_HEAT,
    PHILCO_MODE_DRY, PHILCO_MODE_FAN_ONLY, 0x00 /*off*/
};
static const char* mode_names[] = {"COOL", "HEAT", "DRY", "FAN", "OFF"};
static const uint8_t fan_values[] = {
    PHILCO_FAN_AUTO, PHILCO_FAN_LOW,
    PHILCO_FAN_MEDIUM, PHILCO_FAN_HIGH
};
static const char* fan_names[] = {"AUTO", "LOW", "MED", "HIGH"};

// ── Draw callback ──────────────────────────────────
static void philco_draw_callback(Canvas* canvas, void* ctx) {
    PhilcoApp* app = ctx;
    furi_mutex_acquire(app->mutex, FuriWaitForever);

    canvas_clear(canvas);

    // Mode + Temperature (top section)
    canvas_set_font(canvas, FontPrimary);
    canvas_draw_str(canvas, 2, 12, mode_names[app->settings.mode_index]);

    // Temperature (large, right-aligned)
    char temp_str[8];
    snprintf(temp_str, sizeof(temp_str), "%d°C", app->settings.temperature);
    canvas_set_font(canvas, FontBigNumbers);
    canvas_draw_str_aligned(canvas, 126, 2, AlignRight, AlignTop, temp_str);

    // Temperature bar
    canvas_set_font(canvas, FontSecondary);
    uint8_t bar_width = (app->settings.temperature - PHILCO_TEMP_MIN) * 100
                        / (PHILCO_TEMP_MAX - PHILCO_TEMP_MIN);
    canvas_draw_rframe(canvas, 2, 28, 104, 8, 2);
    canvas_draw_rbox(canvas, 2, 28, bar_width, 8, 2);

    // Fan speed
    canvas_set_font(canvas, FontSecondary);
    char fan_str[24];
    snprintf(fan_str, sizeof(fan_str), "Fan: %s", fan_names[app->settings.fan_index]);
    canvas_draw_str(canvas, 2, 48, fan_str);

    // Swing
    canvas_draw_str(canvas, 70, 48, app->settings.swing_on ? "Swing: ON" : "Swing: OFF");

    // Sending feedback
    if(app->sending) {
        canvas_set_font(canvas, FontPrimary);
        canvas_draw_str_aligned(canvas, 64, 32, AlignCenter, AlignCenter, ">>> SENDING >>>");
    }

    // Button hints
    canvas_set_font(canvas, FontSecondary);
    canvas_draw_str(canvas, 2, 62, "[OK]Send [Hold]Swing");

    furi_mutex_release(app->mutex);
}

// ── Input callback ─────────────────────────────────
static void philco_input_callback(InputEvent* event, void* ctx) {
    PhilcoApp* app = ctx;
    furi_message_queue_put(app->event_queue, event, FuriWaitForever);
}

// ── Transmit current state ─────────────────────────
static void philco_send(PhilcoApp* app) {
    uint8_t packet[PHILCO_PACKET_SIZE];
    bool is_off = (app->settings.mode_index == 4);

    philco_build_packet(
        packet,
        app->settings.temperature,
        is_off ? PHILCO_MODE_COOL : mode_values[app->settings.mode_index],
        fan_values[app->settings.fan_index],
        is_off,
        false
    );

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

static void philco_send_swing(PhilcoApp* app) {
    uint8_t packet[PHILCO_PACKET_SIZE];

    philco_build_packet(
        packet,
        app->settings.temperature,
        mode_values[app->settings.mode_index],
        fan_values[app->settings.fan_index],
        false,
        true  // swing toggle
    );

    uint32_t timings[PHILCO_TOTAL_TIMINGS];
    size_t count;
    philco_packet_to_raw(packet, timings, &count);
    philco_transmit(timings, count);
}

// ── Settings persistence ───────────────────────────
static void philco_load_settings(PhilcoApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_READ, FSOM_OPEN_EXISTING)) {
        storage_file_read(file, &app->settings, sizeof(PhilcoSettings));
    } else {
        // Defaults
        app->settings.temperature = 24;
        app->settings.mode_index = 0;  // Cool
        app->settings.fan_index = 0;   // Auto
        app->settings.swing_on = false;
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

static void philco_save_settings(PhilcoApp* app) {
    Storage* storage = furi_record_open(RECORD_STORAGE);
    File* file = storage_file_alloc(storage);

    if(storage_file_open(file, SETTINGS_PATH, FSAM_WRITE, FSOM_CREATE_ALWAYS)) {
        storage_file_write(file, &app->settings, sizeof(PhilcoSettings));
    }

    storage_file_close(file);
    storage_file_free(file);
    furi_record_close(RECORD_STORAGE);
}

// ── Entry point ────────────────────────────────────
int32_t philco_ac_app(void* p) {
    UNUSED(p);

    PhilcoApp* app = malloc(sizeof(PhilcoApp));
    app->sending = false;
    app->mutex = furi_mutex_alloc(FuriMutexTypeNormal);
    app->event_queue = furi_message_queue_alloc(8, sizeof(InputEvent));

    philco_load_settings(app);

    // GUI setup
    app->view_port = view_port_alloc();
    view_port_draw_callback_set(app->view_port, philco_draw_callback, app);
    view_port_input_callback_set(app->view_port, philco_input_callback, app);
    app->gui = furi_record_open(RECORD_GUI);
    gui_add_view_port(app->gui, app->view_port, GuiLayerFullscreen);

    // Main loop
    InputEvent event;
    bool running = true;

    while(running) {
        if(furi_message_queue_get(app->event_queue, &event, 100) == FuriStatusOk) {
            furi_mutex_acquire(app->mutex, FuriWaitForever);

            if(event.type == InputTypeShort) {
                switch(event.key) {
                    case InputKeyUp:
                        // Cycle mode forward
                        app->settings.mode_index = (app->settings.mode_index + 1) % 5;
                        break;
                    case InputKeyDown:
                        // Cycle mode backward
                        app->settings.mode_index = (app->settings.mode_index + 4) % 5;
                        break;
                    case InputKeyLeft:
                        if(app->settings.temperature > PHILCO_TEMP_MIN)
                            app->settings.temperature--;
                        break;
                    case InputKeyRight:
                        if(app->settings.temperature < PHILCO_TEMP_MAX)
                            app->settings.temperature++;
                        break;
                    case InputKeyOk:
                        app->sending = true;
                        philco_send(app);
                        break;
                    case InputKeyBack:
                        running = false;
                        break;
                    default:
                        break;
                }
            } else if(event.type == InputTypeLong && event.key == InputKeyOk) {
                // Long press OK = toggle swing
                app->settings.swing_on = !app->settings.swing_on;
                philco_send_swing(app);
            } else if(event.type == InputTypeLong && event.key == InputKeyBack) {
                running = false;
            }

            // Clear sending flag after brief display
            if(app->sending && event.type == InputTypeRelease) {
                app->sending = false;
            }

            furi_mutex_release(app->mutex);
            view_port_update(app->view_port);
        }
    }

    // Cleanup
    philco_save_settings(app);
    gui_remove_view_port(app->gui, app->view_port);
    furi_record_close(RECORD_GUI);
    view_port_free(app->view_port);
    furi_message_queue_free(app->event_queue);
    furi_mutex_free(app->mutex);
    free(app);

    return 0;
}
```

---

## 7. Build Instructions

### Prerequisites

1. Clone the Flipper Zero firmware:
   ```bash
   git clone --recursive https://github.com/flipperdevices/flipperzero-firmware.git
   cd flipperzero-firmware
   ```

2. Place the app:
   ```bash
   cp -r philco_ac/ applications_user/philco_ac/
   ```

### Build

```bash
./fbt fap_philco_ac
```

Output FAP will be at:
```
build/f7-firmware-D/.extapps/philco_ac.fap
```

### Install

Copy `philco_ac.fap` to Flipper Zero SD card:
```
SD Card/apps/Infrared/philco_ac.fap
```

Or use qFlipper / Flipper Lab to install.

---

## 8. Testing Checklist

- [ ] **Power Off**: send OFF command, verify AC turns off
- [ ] **Cool 24°C Auto**: most common command, test first
- [ ] **Temperature range**: try 18, 24, 30 — verify AC display matches
- [ ] **All modes**: Cool, Heat, Dry, Fan Only
- [ ] **All fan speeds**: Auto, Low, Med, High
- [ ] **Swing toggle**: press twice — should turn on then off
- [ ] **Settings persistence**: close app, reopen — settings restored
- [ ] **Mode OFF greys out controls**: UI hides temp/fan when off

### Debug tip

If the AC doesn't respond, aim the Flipper's IR LED directly at the AC receiver (usually on the front panel near the display). The Flipper's IR LED is less powerful than a standard remote — distance under 3m works best.

---

## 9. Known Limitations & Notes

1. **Swing is a toggle, not absolute** — the app tracks state locally but if someone uses the physical remote, the app's swing state may desync. This is a protocol limitation.

2. **Model compatibility** — protocol was extracted from the **Philco PH9000QFM4**. Other Philco models likely use the same protocol (same 0x56 signature) but this is not guaranteed.

3. **No timer/schedule** — the ESPHome component doesn't expose timer features. If the physical remote has a timer, those bytes (2, 3, 6, 9, 11) may encode timer data and would need to be captured separately.

4. **Byte 5 mutual exclusion** — you cannot send power OFF and swing toggle in the same packet. Power OFF (0xC0) takes precedence.

---

## 10. Reference

- **Source repo**: https://github.com/brunoamui/esphome-philco-ac
- **Flipper Zero FAP docs**: https://developer.flipper.net/flipperzero/doxygen/apps_on_sd_card.html
- **IR raw signal API**: `infrared_send_raw_ext()` in `lib/infrared/`
- **Flipper app examples**: `applications/examples/` in firmware repo
