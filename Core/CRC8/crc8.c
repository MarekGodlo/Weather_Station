/**
 * @file crc8.c
 *
 * Created on 8/2/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief CRC8/MAXIM algorithm implementation used by 1-Wire devices.
 */

#include "crc8.h"

static uint8_t crc8_maxim_update(uint8_t crc, uint8_t byte);

uint8_t CRC8_Maxim(const uint8_t *data, const uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++)
        crc = crc8_maxim_update(crc, data[i]);
    return crc;
}

static uint8_t crc8_maxim_update(uint8_t crc, uint8_t byte) {
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t b = crc ^ byte;
        crc >>= 1;
        if (b & 0x01)
            crc ^= 0x8c;
        byte >>= 1;
    }
    return crc;
}