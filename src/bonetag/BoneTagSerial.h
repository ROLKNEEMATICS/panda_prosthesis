#ifndef BONETAGSERIAL_H
#define BONETAGSERIAL_H
#include <array>
#include <fstream>
#include "Packet.hpp"
#include <iostream>
#include <mutex>
#include <vector>
#define SENSOR_COUNT 8
namespace io
{
struct BoneTagSerial
{
  using Data = std::array<uint16_t, SENSOR_COUNT>;
  using RawData = std::array<uint16_t, SENSOR_COUNT>;

  BoneTagSerial();
  ~BoneTagSerial();

  void open_serial_port(const std::string & portName, const int baudRate);
  void close_serial_port();
  bool connected();
  void read_serial_port();
  bool isConnected = false;

  std::mutex received_data_mutex;
  std::vector<Data> received_data;

  int cycles_waited  = 0;
  int cycles_timeout = 5000;
  bool serial_status = false;

  bool debug_bytes = true;
  bool debug_raw = true;
  bool debug_results = true;


  inline double alphaFilter() const noexcept{ return alphaFilter_; }
  void alphaFilter(double a) { alphaFilter_ = a; }
protected:

  void parse_buffer(unsigned char *buff, size_t buff_len);
  void apply_filter(std::array<uint16_t, SENSOR_COUNT> & raw_data);
  bool validate_data(std::array<uint16_t, SENSOR_COUNT> & data);

protected:
  int serialPort;
  double alphaFilter_ = 0.3;
  
};
} // namespace io

#endif /* BONETAGSERIAL_H */
