#include "PandaProsthesisController.h"
#include <mc_panda/devices/Robot.h>
#include <mc_rbdyn/RobotLoader.h>

namespace panda_prosthetics
{

PandaProsthetics::PandaProsthetics(mc_rbdyn::RobotModulePtr rm, double dt, const mc_rtc::Configuration & config)
: mc_control::fsm::Controller(rm, dt, config), config_(config)
{
  robots().reserve(100);
  realRobots().reserve(100);
  outputRobots().reserve(100);

  auto robotConfig = config("robots")(robot().name());
  if(robotConfig.has("CollisionBehavior"))
  {
    auto colC = robotConfig("CollisionBehavior");
    mc_rtc::log::warning("[{}] Changing robot CollisionBeaviour to:\n{}", this->name_, colC.dump(true, true));
    auto & robot_device = robot().device<mc_panda::Robot>("Robot");
    robot_device.setCollisionBehavior(colC("lower_torque_thresholds").operator std::array<double, 7>(),
                                      colC("upper_torque_thresholds").operator std::array<double, 7>(),
                                      colC("lower_force_thresholds").operator std::array<double, 6>(),
                                      colC("upper_force_thresholds").operator std::array<double, 6>());
  }

  gui()->addElement(
      {"Frames"}, mc_rtc::gui::Transform("Tibia", [this]() { return robot("panda_tibia").frame("Tibia").position(); }),
      mc_rtc::gui::Transform("Femur", [this]() { return robot("panda_femur").frame("Femur").position(); }),
      mc_rtc::gui::Transform("TibiaCalibration",
                             [this]() { return robot("panda_tibia").frame("TibiaCalibration").position(); }),
      mc_rtc::gui::Transform("FemurCalibration",
                             [this]() { return robot("panda_femur").frame("FemurCalibration").position(); }));
  datastore().make<std::string>("ControllerName", config_("ControllerName", std::string{"bonetag"}));
}

bool PandaProsthetics::run()
{
  // return mc_control::fsm::Controller::run();
  // return mc_control::fsm::Controller::run(mc_solver::FeedbackType::Joints);
  return mc_control::fsm::Controller::run(mc_solver::FeedbackType::ObservedRobots);
}

void PandaProsthetics::reset(const mc_control::ControllerResetData & reset_data)
{
  mc_control::fsm::Controller::reset(reset_data);

  // Set initial pose by attaching the tables together
  if(robot().hasFrame("Front_exterior") && robot("panda_tibia").hasFrame("Right_interior"))
  {
    auto & panda_femur = robot("panda_femur");
    auto & panda_tibia = robot("panda_tibia");
    auto X_0_pt = panda_tibia.posW();

    auto X_pt_Right_interior = panda_tibia.frame("Right_interior").position() * X_0_pt.inv();
    auto X_Right_interior_Front_exterior = sva::RotZ(mc_rtc::constants::PI / 2);
    auto X_Front_exterior_0_pf = X_0_pt * panda_femur.frame("Front_exterior").position().inv();
    auto X_0_pf = X_Front_exterior_0_pf * X_Right_interior_Front_exterior * X_pt_Right_interior * X_0_pt;

    // Manually compensate for most of the calibration offset
    // however the issue is most likely in the robot's kinematics model itself
    auto calibOffsetWorld = sva::PTransformd::Identity();
    // calibOffsetWorld.translation().x() = 0.0;
    // calibOffsetWorld.translation().y() = -0.015;
    // calibOffsetWorld.translation().z() = 0.01;
    X_0_pf = X_0_pf * calibOffsetWorld;

    robot("panda_femur").posW(X_0_pf);
    realRobot("panda_femur").posW(X_0_pf);
    outputRobot("panda_femur").posW(X_0_pf);
  }
}

CONTROLLER_CONSTRUCTOR("PandaProsthesis", panda_prosthetics::PandaProsthetics)

} // namespace panda_prosthetics
