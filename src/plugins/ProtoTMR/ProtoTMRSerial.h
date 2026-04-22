#pragma once

#include "Serial.h"
#include <array>
#include <mutex>
#include <vector>

namespace io
{
struct ProtoTMRSerial : public Serial
{
  constexpr static size_t SENSOR_COUNT = 23;
  constexpr static size_t MEASUREMENTS_PER_SENSOR = 10 + 10;

  ProtoTMRSerial(const std::string & portName, const int baudRate);
  ~ProtoTMRSerial();

  void open_serial_port() override;
  void close_serial_port() override;
  bool connected() override;
  void read_serial_port() override;

protected:
  void parse_buffer(unsigned char * buff, size_t buff_len) override;
  void apply_filter(RawData & raw_data) override;
  bool validate_data(Data & data) override;

  Data avg_buffer;
  std::string line_buffer;
};
} // namespace io
