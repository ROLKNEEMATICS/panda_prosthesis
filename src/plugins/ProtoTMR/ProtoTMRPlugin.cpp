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

  serial_port_name = config("serial_port_name", std::string{"/dev/ttyUSB0"});
  serial_port_baud_rate = config("serial_port_baud_rate", 9600);
  try
  {
    // serial_.reset(new io::ProtoTMRSerial(serial_port_name, serial_port_baud_rate));
  }
  catch(std::exception & e)
  {
    mc_rtc::log::warning("[ProtoTMRPlugin] Could not create ProtoTMR serial, reason: {}", e.what());
  }

  // if(serial_)
  // {
  //   data_.assign(serial_->SENSOR_COUNT, 0);
  // }
  // lastData_ = data_;

  config("verbose", verbose_);

  // FIXME temporary hack to make sensor optional
  bool allowMissingSensor = true;
  serial_ = nullptr;
  // FIXME temporary hack to make sensor optional

  gc.controller().datastore().make<bool>("ProtoTMRPlugin", true);
  gc.controller().datastore().make_call("ProtoTMRPlugin::Connected", [this, allowMissingSensor]() { return allowMissingSensor || (serial_ != nullptr && serial_->connected()); });
  gc.controller().datastore().make_call("ProtoTMRPlugin::RequestNewFrame",
                                        [this]() { if(serial_) serial_->requestNewFrame(); });
  gc.controller().datastore().make_call("ProtoTMRPlugin::GotNewFrame", [this]() { 
        return serial_ ? serial_->gotFullFrame() : true;
        });
  gc.controller().datastore().make_call("ProtoTMRPlugin::GetLastFrame", [this]() { 
      if(serial_) return serial_->getLastFrame();
      else {
        return io::Serial::TimedRawData(io::ProtoTMRSerial::SENSOR_COUNT, io::ProtoTMRSerial::MEASUREMENTS_PER_SENSOR, mc_rtc::clock::now());
      }
  });
  gc.controller().datastore().make_call("ProtoTMRPlugin::Stop", [this]() -> void { running_ = false; });
  // std::vector<std::string> label;
  // {
  //   std::lock_guard<std::mutex> lockDataMutex(dataMutex_);
  //
  //   label.resize(lastData_.size());
  //   for(int i = 0; i < lastData_.size(); ++i)
  //   {
  //     label[i] = std::to_string(i);
  //   }
  // }

  // gc.controller().gui()->addElement(
  //     {"Plugins", "ProtoTMRPlugin"}, mc_rtc::gui::Label("Connected", [this]() { return serial_->connected(); }),
  //     mc_rtc::gui::Button("Connect", [this]() { connect_requested_ = true; }),
  //     mc_rtc::gui::ArrayLabel("Data", label,
  //                             [this]()
  //                             {
  //                               std::lock_guard<std::mutex> lockDataMutex(dataMutex_);
  //                               return lastData_;
  //                             }),
  //     // mc_rtc::gui::NumberInput(
  //     //     "Alpha Filter", [this]() { return serial_->alphaFilter(); }, [this](double a) { serial_->alphaFilter(a);
  //     }),
  //     //
  //     mc_rtc::gui::Button("Stop",
  //                         [this]()
  //                         {
  //                           running_ = false;
  //                           thread_.join();
  //                         }));
}

void ProtoTMRPlugin::reset(mc_control::MCGlobalController & controller) {}

void ProtoTMRPlugin::before(mc_control::MCGlobalController & gc)
{
  // if(!serial_) return;

  // // all_data.insert(all_data.end(),serial_->received_data.begin(),serial_->received_data.end());
  // hasReceivedData_ = !serial_->lastFrameUpdated();

  // if(hasReceivedData_)
  // {
  //   lastData_ = serial_->getLastFrame();
  //   lastDataIsNew_ = true;

  //   if(!plotDisplayed)
  //   {
  //     plotDisplayed = true;
  //     using Color = mc_rtc::gui::Color;
  //     using Style = mc_rtc::gui::plot::Style;
  //     const std::vector<std::pair<Color, Style>> sensorColors = {
  //         {Color(0.8, 0.0, 0.0), Style::Solid}, // Dark Red Solid
  //         {Color(0.8, 0.0, 0.0), Style::Dashed}, // Dark Red Dashed
  //         {Color(0.0, 0.0, 0.8), Style::Solid}, // Dark Blue Solid
  //         {Color(0.0, 0.0, 0.8), Style::Dashed}, // Dark Blue Dashed
  //         {Color(0.0, 0.6, 0.0), Style::Solid}, // Dark Green Solid
  //         {Color(0.0, 0.6, 0.0), Style::Dashed}, // Dark Green Dashed
  //         {Color(0.0, 0.0, 0.0), Style::Solid}, // Black Solid
  //         {Color(0.0, 0.0, 0.0), Style::Dashed}, // Black Dashed
  //         {Color(0.5, 0.0, 0.5), Style::Solid}, // Dark Magenta Solid
  //         {Color(0.5, 0.0, 0.5), Style::Dashed}, // Dark Magenta Dashed
  //         {Color(0.0, 0.5, 0.5), Style::Solid}, // Teal Solid
  //         {Color(0.0, 0.5, 0.5), Style::Dashed}, // Teal Dashed
  //         {Color(0.8, 0.4, 0.0), Style::Solid}, // Dark Orange Solid
  //         {Color(0.8, 0.4, 0.0), Style::Dashed}, // Dark Orange Dashed
  //         {Color(0.5, 0.2, 0.7), Style::Solid}, // Indigo Solid
  //         {Color(0.5, 0.2, 0.7), Style::Dashed}, // Indigo Dashed
  //         {Color(0.5, 0.3, 0.0), Style::Solid}, // Brown Solid
  //         {Color(0.5, 0.3, 0.0), Style::Dashed}, // Brown Dashed
  //         {Color(0.2, 0.2, 0.6), Style::Solid}, // Navy Solid
  //         {Color(0.2, 0.2, 0.6), Style::Dashed} // Navy Dashed
  //     };

  //     // auto make_sensor_plot = [this, sensorColors](unsigned index)
  //     // {
  //     //   const auto & colorStyle = sensorColors[index % sensorColors.size()];
  //     //   return mc_rtc::gui::plot::Y(
  //     //       fmt::format("Sensor {}", index), [this, index]() { return lastData_.data[index]; }, colorStyle.first,
  //     //       colorStyle.second);
  //     // };
  //     //
  //     // auto & gui = *gc.controller().gui();
  //     // // Add buttons to add/remove the "BoneTag Measurements" plot
  //     // gui.addElement({"Plugins", "ProtoTMRPlugin"}, mc_rtc::gui::ElementsStacking::Horizontal,
  //     //                mc_rtc::gui::Button("Add BoneTag Measurements Plot",
  //     //                                    [this, &gui, make_sensor_plot]()
  //     //                                    {
  //     //                                      gui.addPlot("BoneTag Measurements",
  //     //                                                  mc_rtc::gui::plot::X("t", [this]() { return t_; }));
  //     //                                      for(unsigned i = 0; i < serial_->SENSOR_COUNT; ++i)
  //     //                                      {
  //     //                                        gui.addPlotData("BoneTag Measurements", make_sensor_plot(i));
  //     //                                      }
  //     //                                    }),
  //     //                mc_rtc::gui::Button("Remove BoneTag Measurements Plot",
  //     //                                    [this, &gui]() { gui.removePlot("BoneTag Measurements"); }));
  //     //
  //     // for(unsigned i = 0; i < serial_->SENSOR_COUNT; ++i)
  //     // {
  //     //   gui.addElement({"Plugins", "ProtoTMRPlugin"}, mc_rtc::gui::ElementsStacking::Horizontal,
  //     //                  mc_rtc::gui::Button(fmt::format("Add Sensor {}", i),
  //     //                                      [this, &gui, make_sensor_plot, i]()
  //     //                                      {
  //     //                                        gui.addPlot(fmt::format("BoneTag Measurements / Sensor {}", i),
  //     //                                                    mc_rtc::gui::plot::X("t", [this]() { return t_; }),
  //     //                                                    make_sensor_plot(i));
  //     //                                      }),
  //     //                  mc_rtc::gui::Button(fmt::format("Remove Sensor {}", i), [this, &gui, i]()
  //     //                                      { gui.removePlot(fmt::format("BoneTag Measurements / Sensor {}", i)); }));
  //     // }

  //     // std::vector<std::vector<double>> data;
  //     // data.resize(lastData_.data.size());
  //     // gc.controller().logger().addLogEntry("BoneTag_Sensors", this,
  //     //                                      [this]()
  //     //                                      {
  //     //                                        return lastData_.data;
  //     //                                      });
  //   }
  // }
  // hasReceivedData_ = false;
}

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
