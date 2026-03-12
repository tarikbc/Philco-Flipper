#include "philco_ac_protocol.h"
#include <infrared_transmit.h>

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
    bool swing_toggle) {
    for(int i = 0; i < PHILCO_PACKET_SIZE; i++) packet[i] = 0;

    packet[0] = PHILCO_SIGNATURE;

    uint8_t temp = temperature;
    if(temp < PHILCO_TEMP_MIN) temp = PHILCO_TEMP_MIN;
    if(temp > PHILCO_TEMP_MAX) temp = PHILCO_TEMP_MAX;
    packet[1] = 0x6E + (temp - PHILCO_TEMP_MIN);

    packet[2] = 0x00;
    packet[3] = 0x00;
    packet[4] = mode | fan;

    if(power_off) {
        packet[5] = 0xC0;
    } else if(swing_toggle) {
        packet[5] = 0x02;
    } else {
        packet[5] = 0x00;
    }

    packet[6] = 0x00;
    packet[7] = 0x0A;
    packet[8] = 0x0C;
    packet[9] = 0x00;
    packet[10] = 0x0C;
    packet[11] = 0x00;
    packet[12] = 0x1E;
    packet[13] = 0x11;

    packet[14] = philco_calc_checksum(packet);
}

void philco_build_display_toggle_packet(
    uint8_t* packet,
    uint8_t temperature,
    uint8_t mode,
    uint8_t fan) {
    for(int i = 0; i < PHILCO_PACKET_SIZE; i++) packet[i] = 0;

    // Bytes 0, 1, 4 use current AC state (same as normal packet)
    packet[0] = PHILCO_SIGNATURE;

    uint8_t temp = temperature;
    if(temp < PHILCO_TEMP_MIN) temp = PHILCO_TEMP_MIN;
    if(temp > PHILCO_TEMP_MAX) temp = PHILCO_TEMP_MAX;
    packet[1] = 0x6E + (temp - PHILCO_TEMP_MIN);

    packet[2] = 0x00;
    packet[3] = 0x08;  // Display toggle flag
    packet[4] = mode | fan;
    packet[5] = 0x0E;  // Display command bits
    packet[6] = 0x00;
    packet[7] = 0x00;  // Overridden from normal 0x0A
    packet[8] = 0x00;  // Overridden from normal 0x0C
    packet[9] = 0x00;
    packet[10] = 0x00; // Overridden from normal 0x0C
    packet[11] = 0x17; // Overridden from normal 0x00
    packet[12] = 0x23; // Overridden from normal 0x1E
    packet[13] = 0x07; // Overridden from normal 0x11

    packet[14] = philco_calc_checksum(packet);
}

void philco_packet_to_raw(
    const uint8_t* packet,
    uint32_t* timings,
    size_t* timings_count) {
    size_t idx = 0;

    timings[idx++] = PHILCO_HEADER_MARK;
    timings[idx++] = PHILCO_HEADER_SPACE;

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

    timings[idx++] = PHILCO_FOOTER_MARK;
    timings[idx++] = PHILCO_FOOTER_SPACE;

    *timings_count = idx;
}

void philco_transmit(const uint32_t* timings, size_t count) {
    infrared_send_raw_ext(
        timings,
        count,
        true, // start_from_mark
        PHILCO_CARRIER_FREQ,
        PHILCO_DUTY_CYCLE);
}
