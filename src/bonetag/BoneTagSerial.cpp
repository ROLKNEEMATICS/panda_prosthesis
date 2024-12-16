#include "BoneTagSerial.h"
#include <mc_rtc/io_utils.h>
#include <mc_rtc/logging.h>

namespace io
{
#define SYNC_FOUND prev_byte == 'A' && curr_byte == 'T'
#define NUM_BYTES 8
uint8_t prev_byte = 0;
uint8_t curr_byte = 0;
std::array<uint8_t, NUM_BYTES> input_data;

BoneTagSerial::BoneTagSerial()
{
  rawData.fill(0);
}

BoneTagSerial::~BoneTagSerial()
{
  close();
}

void BoneTagSerial::open(const std::string & descriptor)
{
  if(f.is_open())
  {
    f.close();
  }
  f.open(descriptor, std::fstream::in);
  if(!f.is_open())
  {
    throw std::runtime_error(fmt::format("[BoneTagSerial] Failed to open file descriptor {}", descriptor));
  }
  descriptor_ = descriptor;
}

void BoneTagSerial::close()
{
  f.close();
}

bool BoneTagSerial::connected() const noexcept
{
  return f.is_open() && f.good();
}
void BoneTagSerial::sync()
{
  while(true || f.good())
  {
    curr_byte = f.get();

    if(SYNC_FOUND)
    {
      return;
    }
    prev_byte = curr_byte;
  }
  if(!f.good())
  {
    throw std::runtime_error(fmt::format("[BoneTagSerial] Failed to sync (stream error flags are set)"));
  }
}
void BoneTagSerial::print_input_data()
{
  std::cout << "[";
  for(size_t i = 0; i < NUM_BYTES; i++)
  {
    std::cout << "\033[33m" << (int)input_data[i] << "\033[0m";
    if(i < NUM_BYTES - 1)
    {
      std::cout << " , ";
    }
  }
  std::cout << "]" << std::endl;
}
void BoneTagSerial::get_input_data(bool print_bytes)
{
  char rdata[8];
  f.read(rdata, NUM_BYTES);
  if(f.fail())
  {
    throw std::runtime_error("Failed to read");
  }

  for(size_t i = 0; i < NUM_BYTES; i++)
  {
    input_data[i] = rdata[i];
  }
  if(print_bytes)
  {
    print_input_data();
  }
}
void BoneTagSerial::parse_data(bool print_raw_data)
{
  unsigned compt = 0;
  for(size_t i = 0; i < NUM_BYTES / 2; i++)
  {
    uint16_t currentRawData = input_data[compt] << 8;
    currentRawData += input_data[compt + 1];
    rawData[i] = currentRawData;

    compt += 2;
  }
  if(print_raw_data)
  {
    std::cout << "rawData: " << mc_rtc::io::to_string(rawData) << std::endl;
  }
}

//template<typename T>
//T diff(const T&a, const T&b) {
//  return (a > b) ? (a - b) : (b - a);
//}

void BoneTagSerial::parse_result(bool print_result)
{
  result = rawData;
  if(print_result)
  {
    for(size_t i = 0; i < result.size(); i++)
    {
      std::cout << "Result[" << i << "] = " << result[i] << std::endl;
    }
  }
}
bool BoneTagSerial::get_results(bool print_bytes, bool print_raw, bool print_result)
{
  get_input_data(print_bytes);
  parse_data(print_raw);
  parse_result(print_result);

  // Check if data is valid
  for(auto & data : result)
  {
    if(data > 4100)
    {
      mc_rtc::log::warning("Invalid read (read {}, max {})", data, 4100);
      return false;
    }
  }
  return true;
}
const BoneTagSerial::Data & BoneTagSerial::read()
{
  if(f.good())
  {
    sync();
    if(!get_results(debug_bytes, debug_raw, debug_results))
    {
      mc_rtc::log::info("Retrying to read");
      return read();
    }
  }
  else {
    throw std::runtime_error(fmt::format("[BoneTagSerial] Failed to read (stream error flags are set)"));
  }

  return result;
}

} // namespace io
