/**
 * @file crc8.h
 *
 * Created on 8/2/26.
 * @author Marek Godlowicz https://github.com/MarekGodlo
 * @brief CRC8/MAXIM algorithm implementation used by 1-Wire devices.
 */

#ifndef WEATHER_STATION_CRC8_H
#define WEATHER_STATION_CRC8_H
#include <stdint.h>

/**
 * @brief Calculates CRC8/MAXIM checksum.
 *
 * @param data Pointer to the input data buffer.
 * @param len  Length of the buffer.
 * @return Calculated 8-bit CRC value.
 */
uint8_t CRC8_Maxim(const uint8_t *data, uint8_t len);
#endif //WEATHER_STATION_CRC8_H
