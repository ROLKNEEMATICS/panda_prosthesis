#include "ProtoTMRPlugin.h"
// #include "ProtoTMR.h"
#include "ProtoTMRSerial.h"

#include <mc_control/GlobalPluginMacros.h>
#include <mc_rtc/logging.h>
#include <optional>

namespace mc_plugin
{

constexpr auto DEFAULT_RATE = 200; // [Hz]

ProtoTMRPlugin::ProtoTMRPlugin() {}

ProtoTMRPlugin::~ProtoTMRPlugin() {}

void ProtoTMRPlugin::connect() {}

void ProtoTMRPlugin::connectAndStartReading() {}

void ProtoTMRPlugin::init(mc_control::MCGlobalController & gc, const mc_rtc::Configuration & config)
{
  auto & ctl = gc.controller();

  config_ = config;
  if(auto ctlConfig = ctl.config().find("ProtoTMRPlugin"))
  {
    config_.load(*ctlConfig);
  };

  serial_port_name = config_("serial_port_name", std::string{"/dev/ttyUSB0"});
  serial_port_baud_rate = config_("serial_port_baud_rate", 9600);
  config_("verbose", verbose_);
  config_("sensorRequired", sensorRequired_);
  try
  {
    serial_.reset(new io::ProtoTMRSerial(serial_port_name, serial_port_baud_rate));
  }
  catch(const std::exception & e)
  {
    if(sensorRequired_)
    {
      throw;
    }
    mc_rtc::log::warning("[ProtoTMRPlugin] Could not open serial sensor (sensorRequired=false): {}", e.what());
  }
  mc_rtc::log::info("[ProtoTMRPlugin] Initialized with config:\n{}", config_.dump(true, true));

  gc.controller().datastore().make<bool>("ProtoTMRPlugin", true);
  gc.controller().datastore().make_call("ProtoTMRPlugin::Connected",
                                        [this]() { return !sensorRequired_ || (serial_ && serial_->connected()); });
  gc.controller().datastore().make_call("ProtoTMRPlugin::RequestNewFrame",
                                        [this]()
                                        {
                                          if(sensorRequired_ && (!serial_ || !serial_->connected()))
                                          {
                                            mc_rtc::log::error_and_throw(
                                                "[ProtoTMRPlugin::RequestNewFrame] Requesting new frame, but no serial "
                                                "connection is active and sensorRequired=true");
                                          }
                                          if(serial_ && serial_->connected()) serial_->requestNewFrame();
                                        });
  gc.controller().datastore().make_call("ProtoTMRPlugin::GotNewFrame",
                                        [this]()
                                        {
                                          if(sensorRequired_ && (!serial_ || !serial_->connected()))
                                          {
                                            mc_rtc::log::error_and_throw(
                                                "[ProtoTMRPlugin::GotNewFrame] Requesting new frame, but no serial "
                                                "connection is active and sensorRequired=true");
                                          }
                                          // mc_rtc::log::info("[ProtoTMRSerial::GotNewFrame]: sensorRequired: {},
                                          // gotFullFrame: {}", sensorRequired_, serial_->gotFullFrame());
                                          return sensorRequired_ ? serial_->gotFullFrame() : true;
                                        });
  gc.controller().datastore().make_call(
      "ProtoTMRPlugin::GetLastFrame",
      [this]()
      {
        if(sensorRequired_ && (!serial_ || !serial_->connected()))
        {
          mc_rtc::log::error_and_throw("[ProtoTMRPlugin::GetLastFrame] Requesting last frame, but no serial connection "
                                       "is active and sensorRequired=true");
        }
        if(serial_ && serial_->connected())
          return serial_->getLastFrame();
        else
        {
          return io::Serial::TimedRawData(io::ProtoTMRSerial::SENSOR_COUNT, io::ProtoTMRSerial::MEASUREMENTS_PER_SENSOR,
                                          mc_rtc::clock::now());
        }
      });
  gc.controller().datastore().make_call("ProtoTMRPlugin::Stop",
                                        [this]() -> void
                                        {
                                          if(serial_) serial_->disconnect();
                                        });
}

void ProtoTMRPlugin::reset(mc_control::MCGlobalController & /* controller */) {}

void ProtoTMRPlugin::before(mc_control::MCGlobalController & /* gc */) {}

void ProtoTMRPlugin::after(mc_control::MCGlobalController & controller)
{
  t_ += controller.timestep();
}

mc_control::GlobalPlugin::GlobalPluginConfiguration ProtoTMRPlugin::configuration()
{
  mc_control::GlobalPlugin::GlobalPluginConfiguration out;
  out.should_run_before = true;
  out.should_run_after = true;
  out.should_always_run = false;
  return out;
}

} // namespace mc_plugin

EXPORT_MC_RTC_PLUGIN("ProtoTMRPlugin", mc_plugin::ProtoTMRPlugin)
