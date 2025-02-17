#include "Packet.hpp"
#include <iostream>
#include <vector>

static std::array<uint32_t, 256> crc32_table; ///< Lookup table for CRC32 calculations

/**
 * @brief Initializes the CRC32 lookup table using a polynomial.
 */
void init_crc32_table()
{

  constexpr uint32_t polynomial = 0xEDB88320; ///< Polynomial used for CRC32
  for(uint32_t i = 0; i < 256; ++i)
  {
    uint32_t crc = i;
    for(uint8_t j = 0; j < 8; ++j)
    {
      crc = (crc >> 1) ^ (crc & 1 ? polynomial : 0); ///< Update CRC value
    }
    crc32_table[i] = crc; ///< Store the computed CRC value in the table
  }

}

/**
 * @brief Computes the CRC32 checksum for the provided data.
 *
 * @param data Pointer to the data buffer.
 * @param length Length of the data in bytes.
 * @return Computed CRC32 checksum.
 */
uint32_t calculate_crc32(const uint8_t * data, size_t length)
{

  uint32_t crc = ~0U; ///< Initialize CRC value to all 1s
  for(size_t i = 0; i < length; ++i)
  {
    crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF]; ///< Update CRC value for each byte
  }
  return ~crc; ///< Finalize CRC value by inverting all bits
}

/**
 * @brief Decodes a packet and validates its content.
 *
 * @param raw_bytes Pointer to the raw bytes.
 * @param raw_bytes_length Size of the raw byte array.
 * @param packet Reference to the packet to decode.
 * @return 0 on success, -1 if wrong packet size, -2 if delimiters are invalid, -3 if CRC validation fails.
 */

std::pair<std::array<uint16_t, SENSOR_COUNT>, int> decode_packet(const uint8_t * raw_bytes, size_t raw_bytes_length)
{
  if(raw_bytes_length != PACKET_SIZE)
  {
    return {{}, -1}; // Invalid packet size
  }

  std::array<uint16_t, SENSOR_COUNT> sensor_values;

  // Copy data into the Packet structure
  std::memcpy(&sensor_values, raw_bytes + 3, sizeof(sensor_values));
  uint32_t crc;
  std::memcpy(&crc, raw_bytes + 3 + sizeof(sensor_values), sizeof(crc));

  // Validate CRC32

  uint32_t computed_crc = calculate_crc32(&raw_bytes[2], sizeof(sensor_values)+1);
  if(computed_crc != crc)
  {
    //std::cerr << "Expected CRC: " << crc << "\tComputed CRC: " << computed_crc << "\n";
    return {{}, -3}; ///< CRC mismatch: data integrity check failed
  }

  return {sensor_values, 0}; ///< Successfully decoded
}
