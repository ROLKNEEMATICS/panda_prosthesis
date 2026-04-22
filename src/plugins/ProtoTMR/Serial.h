#pragma once

#include <mc_rtc/clock.h>
#include <mc_rtc/logging.h>
#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <termios.h>
#include <thread>
#include <vector>

namespace io
{

inline int baud_to_int(speed_t baud)
{
  switch(baud)
  {
    case B0:
      return 0;
    case B50:
      return 50;
    case B75:
      return 75;
    case B110:
      return 110;
    case B134:
      return 134;
    case B150:
      return 150;
    case B200:
      return 200;
    case B300:
      return 300;
    case B600:
      return 600;
    case B1200:
      return 1200;
    case B1800:
      return 1800;
    case B2400:
      return 2400;
    case B4800:
      return 4800;
    case B9600:
      return 9600;
    case B19200:
      return 19200;
    case B38400:
      return 38400;
    case B57600:
      return 57600;
    case B115200:
      return 115200;
    case B230400:
      return 230400;
    case B460800:
      return 460800;
    case B500000:
      return 500000;
    case B576000:
      return 576000;
    case B921600:
      return 921600;
    case B1000000:
      return 1000000;
    case B1152000:
      return 1152000;
    case B1500000:
      return 1500000;
    case B2000000:
      return 2000000;
    case B2500000:
      return 2500000;
    case B3000000:
      return 3000000;
    case B3500000:
      return 3500000;
    case B4000000:
      return 4000000;
    default:
      return -1; // Unknown baud rate
  }
}
inline speed_t int_to_baud(int baud)
{
  switch(baud)
  {
    case 0:
      return B0;
    case 50:
      return B50;
    case 75:
      return B75;
    case 110:
      return B110;
    case 134:
      return B134;
    case 150:
      return B150;
    case 200:
      return B200;
    case 300:
      return B300;
    case 600:
      return B600;
    case 1200:
      return B1200;
    case 1800:
      return B1800;
    case 2400:
      return B2400;
    case 4800:
      return B4800;
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    case 230400:
      return B230400;
    case 460800:
      return B460800;
    case 500000:
      return B500000;
    case 576000:
      return B576000;
    case 921600:
      return B921600;
    case 1000000:
      return B1000000;
    case 1152000:
      return B1152000;
    case 1500000:
      return B1500000;
    case 2000000:
      return B2000000;
    case 2500000:
      return B2500000;
    case 3000000:
      return B3000000;
    case 3500000:
      return B3500000;
    case 4000000:
      return B4000000;
    default:
      return (speed_t)-1; // Unknown baud rate
  }
}

template<typename DataT>
struct TimedData
{
  using SensorDataT = typename DataT::value_type;
  TimedData() {}
  TimedData(unsigned int sensor_count,
            unsigned int meaurementsPerSensor,
            std::chrono::time_point<mc_rtc::clock> initial_time = mc_rtc::clock::now())
  : start_time_ms(0.0), end_time_ms(0.0), initial_time(initial_time), data{}
  {
    data.resize(sensor_count);
    for(auto & sensor_data : data)
    {
      sensor_data.resize(meaurementsPerSensor);
    }
  }

  /**
   * \brief Reset the initial time to the current time, that is the time at which the first measurement was started
   */
  void resetInitialTime()
  {
    initial_time = mc_rtc::clock::now();
  };

  /**
   * \brief Set the start time of the current frame to the current time
   */
  void startFrame()
  {
    auto now = mc_rtc::clock::now();
    start_time_ms = std::chrono::duration_cast<mc_rtc::duration_ms>(now - initial_time);
  }

  /**
   * \brief Set the end time of the current frame to the current time and compute the duration of the frame
   */
  void finalizeFrame()
  {
    auto now = mc_rtc::clock::now();
    end_time_ms = std::chrono::duration_cast<mc_rtc::duration_ms>(now - initial_time);
  }

  /**
   * Moves a full frame data
   */
  TimedData(DataT && data)
  {
    this->data = std::move(data);
  }

  mc_rtc::duration_ms start_time_ms{0.0}; /// time at which the first sensor in the current frame began reading
  mc_rtc::duration_ms end_time_ms{0.0}; /// time at which the last sensor in the current frame finished reading
  std::chrono::time_point<mc_rtc::clock> initial_time; /// time at which the first measurement was started
  DataT data; /// sensor data for a full frame
};

struct Serial
{
  size_t SENSOR_COUNT = 32;
  std::chrono::time_point<mc_rtc::clock> start_time_;

  using Data = std::vector<uint64_t>;
  using RawData = std::vector<std::vector<uint64_t>>;

  using TimedFilteredData = TimedData<Data>;
  using TimedRawData = TimedData<RawData>;

  std::string portName = "";
  const int baudRate = 115200;

  Serial(const std::string & portName, const int baudRate, size_t sensor_count, size_t meaurementsPerSensor)
  : SENSOR_COUNT(sensor_count), start_time_(mc_rtc::clock::now()),
    lastSensorFrame_(sensor_count, meaurementsPerSensor, start_time_), currentSensorFrame_(lastSensorFrame_),
    portName(portName), baudRate(baudRate)
  {
    thread_ = std::thread(&Serial::runThread, this);
  };

  virtual void open_serial_port() = 0;
  virtual void close_serial_port() = 0;
  virtual bool connected() = 0;
  virtual void read_serial_port() = 0;
  bool running() const noexcept
  {
    return running_;
  }

  void disconnect()
  {
    running_ = false;
  }

  inline bool lastFrameUpdated() const noexcept
  {
    return frameUpdated_;
  }

  /**
   * \brief Request a new frame. You must wait until gotFullFrame() = true to have the new full sensor frame
   */
  void requestNewFrame()
  {
    gotFullFrame_ = false;
  }

  /**
   * \bried True after a full frame has been received:
   * - if you called requestNewFrame(): this is true when the next full frame has been received
   * - otherwise it always true after the first full sensor frame has been received
   **/
  inline bool gotFullFrame() const noexcept
  {
    return gotFullFrame_;
  }

  /**
   * \brief True if the frame has changed since the last call to getLastFrame()
   */
  inline bool frameUpdated() const noexcept
  {
    return frameUpdated_;
  }

  /**
   * \brief Get the last full sensor frame that has been received.
   * Call frameUpdated() beforehand to check if the frame has changed since the last call to this function.
   */
  TimedRawData getLastFrame() const
  {
    frameUpdated_ = false;
    std::lock_guard<std::mutex> lock(frameMutex_);
    return lastSensorFrame_;
  }

protected: /* Serial stream processing */
  virtual void parse_buffer(unsigned char * buff, size_t buff_len) = 0;
  virtual void apply_filter(RawData & raw_data) = 0;
  virtual bool validate_data(Data & data) = 0;

protected: /* Sensor data */
  // processed in a thread, do not use outside
  // all received raw data since we last read it with popRawData()

  /**
   * Frame synchronization mutex. Derived classes must lock it before writing into lastSensorFrame_
   */
  mutable std::mutex frameMutex_;
  TimedRawData lastSensorFrame_; /// last available full sensor frame
  TimedRawData currentSensorFrame_; /// current sensor frame being processed (incomplete). No locking required as it is
                                    /// never read outside of the thread

  std::atomic<bool> gotFullFrame_{false}; /// True when we got the next full frame
  mutable std::atomic<bool> frameUpdated_{
      false}; /// whether the frame has changed since the last call to getLastFrame()

protected: /* Serial stream thread */
  std::thread thread_;
  bool isConnected = false;

  void runThread()
  {
    running_ = true;
    try
    {
      open_serial_port();
    }
    catch(const std::exception & e)
    {
      mc_rtc::log::error("Failed to open serial port '{}', reason: {}", portName, e.what());
      running_ = false;
    }

    const auto period = std::chrono::milliseconds(1); // 1000Hz = 1ms period

    while(running_)
    {
      auto loop_start = std::chrono::steady_clock::now();

      read_serial_port();

      auto loop_end = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(loop_end - loop_start);

      if(elapsed < period)
      {
        std::this_thread::sleep_for(period - elapsed);
      }
    }

    close_serial_port();
  }

  int cycles_waited = 0;
  int cycles_timeout = 5000;
  bool serial_status = false;

  bool debug_bytes = true;
  bool debug_raw = true;
  bool debug_results = true;

  inline double alphaFilter() const noexcept
  {
    return alphaFilter_;
  }
  void alphaFilter(double a)
  {
    alphaFilter_ = a;
  }

protected:
  bool running_ = false;
  int serialPort;
  double alphaFilter_ = 0.3;
};
} // namespace io
