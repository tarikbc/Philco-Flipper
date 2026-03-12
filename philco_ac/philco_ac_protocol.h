#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define PHILCO_SIGNATURE      0x56
#define PHILCO_PACKET_SIZE    15
#define PHILCO_TOTAL_TIMINGS  244  // 2 + (15*8*2) + 2

#define PHILCO_HEADER_MARK    8400
#define PHILCO_HEADER_SPACE   4200
#define PHILCO_BIT_MARK       600
#define PHILCO_BIT_ONE_SPACE  1600
#define PHILCO_BIT_ZERO_SPACE 550
#define PHILCO_FOOTER_MARK    600
#define PHILCO_FOOTER_SPACE   10000

#define PHILCO_CARRIER_FREQ   38000
#define PHILCO_DUTY_CYCLE     0.33f

#define PHILCO_TEMP_MIN       18
#define PHILCO_TEMP_MAX       30

// Mode values (upper nibble of byte 4)
#define PHILCO_MODE_HEAT      0x10
#define PHILCO_MODE_COOL      0x20
#define PHILCO_MODE_DRY       0x30
#define PHILCO_MODE_FAN_ONLY  0x40

// Fan values (lower nibble of byte 4)
#define PHILCO_FAN_HIGH       0x00
#define PHILCO_FAN_MEDIUM     0x01
#define PHILCO_FAN_AUTO       0x02
#define PHILCO_FAN_LOW        0x03

// Build a 15-byte packet from the given state
void philco_build_packet(
    uint8_t* packet,
    uint8_t temperature,
    uint8_t mode,
    uint8_t fan,
    bool power_off,
    bool swing_toggle);

// Build a display toggle packet (uses current AC state but overrides specific bytes)
void philco_build_display_toggle_packet(
    uint8_t* packet,
    uint8_t temperature,
    uint8_t mode,
    uint8_t fan);

// Convert 15-byte packet to raw IR timings array
void philco_packet_to_raw(
    const uint8_t* packet,
    uint32_t* timings,
    size_t* timings_count);

// Transmit using Flipper's IR subsystem
void philco_transmit(const uint32_t* timings, size_t count);
