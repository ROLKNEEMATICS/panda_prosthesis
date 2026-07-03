#include "Calibrate.h"

#include <mc_control/fsm/Controller.h>
#include <utils.h>

// XXX it would make more sense to store the pose corresponding to the
// calibrated joint without the calibration tool,
// e.g the one after PrepareJoint
void save(const std::string & etc_file, const mc_rbdyn::Robot & robot, bool savePose)
{
  mc_rtc::Configuration initial(etc_file);
  initial.add(robot.name());
  if(savePose)
  {
    initial(robot.name()).add("pose", robot.posW());
  }
  initial(robot.name()).add("joints", robot.mbc().q);
  initial.save(etc_file);
  mc_rtc::log::success("Calibration saved to {}", etc_file);
}

void Calibrate::start(mc_control::fsm::Controller & ctl)
{
  config_("savePose", savePose_);
  ctl.gui()->addElement(this, {"Calibration"},
                        mc_rtc::gui::Checkbox(
                            "Save base pose", [this]() { return savePose_; }, [this]() { savePose_ = !savePose_; }),
                        mc_rtc::gui::Button("Save",
                                            [this]()
                                            {
                                              save_ = true;
                                              next_ = true;
                                            }),
                        mc_rtc::gui::Button("Ignore",
                                            [this]()
                                            {
                                              save_ = false;
                                              next_ = true;
                                            }));

  output("OK");

  // Computes calibration online
  run(ctl);
}

bool Calibrate::run(mc_control::fsm::Controller & ctl)
{
  return next_;
}

void Calibrate::teardown(mc_control::fsm::Controller & ctl)
{
  if(savePose_)
  {
    // 1. Compute relative pose between the panda_tibia/panda_femur floating base
    // taking panda_tibia as the (arbitrary) reference
    auto & panda_femur = ctl.realRobot("panda_femur");
    auto & panda_tibia = ctl.realRobot("panda_tibia");
    auto X_0_pt = panda_tibia.posW();
    auto X_0_pf = panda_femur.posW();
    auto X_pt_tibia = panda_tibia.frame("TibiaCalibration").position() * X_0_pt.inv();
    auto X_pf_femur = panda_femur.frame("FemurCalibration").position() * X_0_pf.inv();
    auto X_pt_pf = X_pf_femur.inv() * X_pt_tibia;
    X_0_pf = X_pt_pf * X_0_pt;

    // 2. Set the panda_femur floating base
    ctl.realRobot("panda_femur").posW(X_0_pf);
    ctl.robot("panda_femur").posW(X_0_pf);
  }

  // if(config_("save", true))
  if(save_)
  {
    auto controllerName = ctl.datastore().get<std::string>("ControllerName");
    auto etc_dir = get_or_create_dir("calibration/" + controllerName);
    auto etc_file = [&ctl, etc_dir](const mc_rbdyn::Robot & robot) -> std::string
    { return fmt::format("{}/initial_{}.yaml", etc_dir, robot.module().parameters()[0]); };
    save(etc_file(ctl.robot("panda_femur")), ctl.robot("panda_femur"), savePose_);
    save(etc_file(ctl.robot("panda_tibia")), ctl.robot("panda_tibia"), savePose_);
  }

  ctl.gui()->removeElements(this);
}

EXPORT_SINGLE_STATE("Calibrate", Calibrate)
