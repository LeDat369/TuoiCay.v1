/**
 * @file crc_utils.h
 * @brief CRC calculation utilities for data verification
 * 
 * LOGIC:
 * - CRC8 for small data (config structs)
 * - CRC16 for larger data (files)
 * - Used to verify config integrity after storage read
 * 
 * RULES: #NVS(18) - Storage verification
 */

#ifndef CRC_UTILS_H
#define CRC_UTILS_H

#include <stdint.h>
#include <stddef.h>

//=============================================================================
// CRC8 (Dallas/Maxim polynomial 0x31)
//=============================================================================
/**
 * @brief Calculate CRC8 checksum
 * @param data Pointer to data buffer
 * @param len Length of data
 * @return CRC8 checksum
 */
inline uint8_t crc8(const uint8_t* data, size_t len) {
    uint8_t crc = 0x00;
    while (len--) {
        crc ^= *data++;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x80) {
                crc = (crc << 1) ^ 0x31;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

//=============================================================================
// CRC16 (CCITT polynomial 0x1021)
//=============================================================================
/**
 * @brief Calculate CRC16-CCITT checksum
 * @param data Pointer to data buffer
 * @param len Length of data
 * @return CRC16 checksum
 */
inline uint16_t crc16(const uint8_t* data, size_t len) {
    uint16_t crc = 0xFFFF;
    while (len--) {
        crc ^= (uint16_t)(*data++) << 8;
        for (uint8_t i = 0; i < 8; i++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

//=============================================================================
// HELPER MACROS
//=============================================================================

/**
 * @brief Calculate CRC8 for a struct (excluding the CRC field at the end)
 * @param structPtr Pointer to struct
 * @param structType Type of struct
 * @return CRC8 of struct data (excluding last byte which is CRC)
 * 
 * USAGE:
 *   struct Config { int a; int b; uint8_t crc; };
 *   Config cfg = {1, 2, 0};
 *   cfg.crc = CRC8_STRUCT(&cfg, Config);
 */
#define CRC8_STRUCT(structPtr, structType) \
    crc8((const uint8_t*)(structPtr), sizeof(structType) - sizeof(uint8_t))

/**
 * @brief Verify CRC8 of a struct
 * @return true if CRC matches, false otherwise
 */
#define CRC8_VERIFY(structPtr, structType) \
    (crc8((const uint8_t*)(structPtr), sizeof(structType) - sizeof(uint8_t)) == \
     ((const uint8_t*)(structPtr))[sizeof(structType) - 1])

#endif // CRC_UTILS_H
