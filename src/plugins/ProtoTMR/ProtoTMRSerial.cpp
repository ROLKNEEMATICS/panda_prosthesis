#include "ProtoTMRSerial.h"
#include <mc_rtc/io_utils.h>
#include <mc_rtc/logging.h>
#include <cstdint>
#include <fcntl.h>
#include <memory>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>
namespace io
{

#define BUFFER_SIZE 256
static unsigned char buffer[BUFFER_SIZE];

// 23 sensors
// 10 timestamps per sensor
// 10 readings per sensor
ProtoTMRSerial::ProtoTMRSerial(const std::string & portName, const int baudRate)
: Serial(portName, baudRate, ProtoTMRSerial::SENSOR_COUNT, ProtoTMRSerial::MEASUREMENTS_PER_SENSOR)
{
  avg_buffer.resize(SENSOR_COUNT, 0);
}

ProtoTMRSerial::~ProtoTMRSerial()
{
  close_serial_port();
}

void ProtoTMRSerial::open_serial_port()
{
  serialPort = open(portName.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if(serialPort < 0)
  {
    throw std::runtime_error(fmt::format("[ProtoTMRSerial] Failed to open serial port \"{}\"", portName));
  }
  // Configure the serial port
  struct termios tty;
  if(tcgetattr(serialPort, &tty) != 0)
  {
    throw std::runtime_error(fmt::format("[ProtoTMRSerial] Error getting terminal attributes"));
    close(serialPort);
  }

  // Set baud rate to 9600
  cfsetispeed(&tty, int_to_baud(baudRate));
  cfsetospeed(&tty, int_to_baud(baudRate));

  // Configure 8N1 (8 data bits, no parity, 1 stop bit)
  tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8; // 8-bit characters
  tty.c_iflag &= ~IGNBRK; // Disable break processing
  tty.c_lflag = 0; // No canonical mode, no echo
  tty.c_oflag = 0; // No remapping, no delays
  tty.c_cc[VMIN] = 1; // Read at least 1 character
  tty.c_cc[VTIME] = 1; // Timeout after 0.1 seconds

  // Apply the settings to the serial port
  if(tcsetattr(serialPort, TCSANOW, &tty) != 0)
  {
    throw std::runtime_error(fmt::format("[ProtoTMRSerial] Error setting terminal attributes!"));
    close(serialPort);
  }
  isConnected = true;
}

void ProtoTMRSerial::close_serial_port()
{
  close(serialPort);
  isConnected = false;
}

bool ProtoTMRSerial::connected()
{
  if(fcntl(serialPort, F_GETFL) < 0)
  {
    isConnected = false;
  }
  return isConnected;
}

void ProtoTMRSerial::read_serial_port()
{
  // mc_rtc::log::info("read serial port called at t={}",
  // std::chrono::duration_cast<mc_rtc::duration_ms>(mc_rtc::clock::now().time_since_epoch()).count());
  char read_buf[128];

  int bytes_available = 0;
  if(ioctl(serialPort, FIONREAD, &bytes_available) == -1)
  {
    mc_rtc::log::error("ioctl FIONREAD failed");
    return;
  }
  if(bytes_available < 128) return;

  int BytesRead = read(serialPort, read_buf, sizeof(read_buf));
  if(BytesRead > 0)
  {
    line_buffer.append(read_buf, BytesRead);
    // auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(mc_rtc::clock::now().time_since_epoch()).count();
    // mc_rtc::log::info("read {} bytes, line_buffer: {}, time: {} ms",
    //                   BytesRead,
    //                   line_buffer,
    //                   ms);
    size_t pos = 0;
    while((pos = line_buffer.find('\n')) != std::string::npos)
    {
      std::string line = line_buffer.substr(0, pos);
      // mc_rtc::log::info("read line: {}", line);
      // Optionally remove carriage return if present
      if(!line.empty() && line.back() == '\r')
      {
        line.pop_back();
      }
      size_t line_len = std::min(line.size(), sizeof(buffer));
      memcpy(buffer, line.data(), line_len);
      parse_buffer(buffer, line_len);
      line_buffer.erase(0, pos + 1); // Remove parsed line (including '\n')
    }
    cycles_waited = 0;
    serial_status = true;
  }
  else
  {
    if(++cycles_waited > cycles_timeout) serial_status = false;
  }
}

void ProtoTMRSerial::apply_filter(RawData & raw_data)
{
  // filtered_current = round(coef * raw_current + (1 - coef) * filtered_previous)
  // size_t index = 0;
  // for(auto & value : raw_data)
  // {
  //   value = round(alphaFilter_ * value + (1 - alphaFilter_) * last_received_data[index]);
  //   index++;
  // }
}

bool ProtoTMRSerial::validate_data(Data & raw_data)
{
  for(auto & value : raw_data)
  {
    if(value > 4100)
    {
      mc_rtc::log::warning("[ProtoTMRSerial] Invalid read (read {}, max {})", value, 4100);
      return false;
    }
  }
  return true;
}

// data is organized as follow
// - 0: id
// - 1: sensor value
// example data is (single line):
// 0 1024
// 1 1000
// ...
// 22 1045
//
// This function is called once for every line
void ProtoTMRSerial::parse_buffer(unsigned char * buff, size_t buff_len)
{
  // mc_rtc::log::info("Parsing buffer of size {}", buff_len);
  std::string line(reinterpret_cast<char *>(buff), buff_len);
  // mc_rtc::log::info("parse buffer got line: {}", line);
  auto numbers = std::vector<uint64_t>{};
  // numbers.reserve(31); // Reserve space for 31 numbers to avoid reallocations
  std::stringstream ss(line);
  std::string item;
  while(std::getline(ss, item, ' '))
  {
    if(item.empty()) continue;
    try
    {
      numbers.emplace_back(static_cast<uint64_t>(std::stoul(item)));
      // mc_rtc::log::info("item: {}, number: {}", item, numbers.back());
    }
    catch(const std::exception &)
    {
      mc_rtc::log::warning("Failed to parse item '{}' as integer, skipping", item);
    }
  }
  // mc_rtc::log::info("size: {}, line: {}", numbers.size(), line);
  if(numbers.size() != 2) // MEASUREMENTS_PER_SENSOR + timestamp
  {
    mc_rtc::log::warning("wrong sensor size, got {} expected at least 2", numbers.size());
    mc_rtc::log::warning("{}", mc_rtc::io::to_string(numbers));
    return;
  }

  auto sensorId = numbers[0] - 1;
  if(sensorId == 0)
  { // Frame start found
    // mc_rtc::log::success("Found frame start");
    currentSensorFrame_.startFrame();
  }

  // copy to the full frame
  currentSensorFrame_.data[sensorId][0] = numbers[1];

  if(sensorId == 22)
  { // last sensor, frame is complete
    currentSensorFrame_.finalizeFrame();
    // mc_rtc::log::info("start frame time: {}s", currentSensorFrame_.start_time_ms.count()/1000);
    // mc_rtc::log::info("end frame time: {}s", currentSensorFrame_.end_time_ms.count()/1000);
    // mc_rtc::log::info("delta time: {}s", currentSensorFrame_.end_time_ms.count()/1000 -
    // currentSensorFrame_.start_time_ms.count()/1000);

    // update lastSensorFrame
    {
      std::lock_guard<std::mutex> lock{frameMutex_};
      lastSensorFrame_ = currentSensorFrame_;
    }
    gotFullFrame_ = true;
    frameUpdated_ = true;
    // mc_rtc::log::success("Full frame received and updated");
  }
}

} // namespace io
