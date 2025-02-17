// packet.h
#ifndef PACKET_H
#define PACKET_H

#include <cstdint>
#include <cstring>
#include <cstdio>
#include <array>
#include <string>
#include <sstream>
#include <cstdint>
#include <iostream>

constexpr char STX[] = "AT";                                 ///< Start of Text delimiter
constexpr char EOT[] = "TA";                                 ///< End of Text delimiter
constexpr size_t SENSOR_COUNT = 8;                           ///< Number of sensors in the data
constexpr size_t PACKET_SIZE = 2 + 1 + (SENSOR_COUNT * 2) + 4 + 2; ///< STX + ConfigByte + Sensor Data + CRC32 + EOT

#pragma pack(push, 1) ///< Ensure packed structure without padding
/**
 * @brief Structure to hold sensor data.
 */
struct SensorData
{
    std::array<uint16_t, SENSOR_COUNT> sensors_data; ///< Array to hold sensor data
    // Method to return a space-separated string representation of the data
    std::string toString() const
    {
        std::ostringstream oss;
        for (size_t i = 0; i < sensors_data.size(); ++i)
        {
            oss << sensors_data[i];
            if (i < sensors_data.size() - 1) // Avoid adding a space after the last element
            {
                oss << ' ';
            }
        }
        return oss.str();
    }
};


#pragma pack(pop)

/**
 * @brief Initializes the CRC32 lookup table.
 */
void init_crc32_table();

/**
 * @brief Computes the CRC32 checksum for the given data.
 *
 * @param data Pointer to the data.
 * @param length Length of the data in bytes.
 * @return Computed CRC32 checksum.
 */
uint32_t calculate_crc32(const uint8_t *data, size_t length);

/**
 * @brief Decodes a packet and validates its content.
 *
 * @param raw_bytes Pointer to the raw bytes.
 * @param raw_bytes_length Size of the raw byte array.
 * @param packet Reference to the packet to decode.
 * @return 0 on success, -1 if wrong packet size, -2 if delimiters are invalid, -3 if CRC validation fails.
 */
std::pair<std::array<uint16_t, SENSOR_COUNT>,int> decode_packet(const uint8_t *raw_bytes, size_t raw_bytes_length);

#endif // PACKET_H