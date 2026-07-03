/*
 * Copyright 2021 CNRS-UM LIRMM
 */

#pragma once

#include <mc_control/GlobalPlugin.h>
#include <mc_rtc/gui/StateBuilder.h>
// #include "ProtoTMR.h"
#include "ProtoTMRSerial.h"
#include <memory>
#include <mutex>
#include <termios.h>
#include <thread>

namespace mc_plugin
{

struct ProtoTMRPlugin : public mc_control::GlobalPlugin
{
  void init(mc_control::MCGlobalController & controller, const mc_rtc::Configuration & config) override;

  void reset(mc_control::MCGlobalController & controller) override;

  void before(mc_control::MCGlobalController &) override;

  void after(mc_control::MCGlobalController & controller) override;

  mc_control::GlobalPlugin::GlobalPluginConfiguration configuration() override;

  ProtoTMRPlugin();
  ~ProtoTMRPlugin() override;

  // Thread functions
protected:
  void connect();
  void connectAndStartReading();

protected:
  mc_rtc::Configuration config_;
  bool sensorRequired_ = true;
  int serial_port_baud_rate = 9600;
  std::string serial_port_name;

  bool connect_requested_ = false;
  std::unique_ptr<io::Serial> serial_;
  std::thread thread_;
  std::mutex dataMutex_;

  io::Serial::Data data_;
  io::Serial::TimedRawData lastData_;
  double t_ = 0;
  bool plotDisplayed = false;

  bool hasReceivedData_ = false;
  bool lastDataIsNew_ = false;

  bool running_ = true;
  bool verbose_ = true;
  unsigned rate_ = 0;
};

} // namespace mc_plugin
