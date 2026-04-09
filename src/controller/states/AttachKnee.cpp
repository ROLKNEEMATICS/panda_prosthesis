#include "AttachKnee.h"
#include <mc_control/fsm/Controller.h>
#include <mc_rbdyn/RobotLoader.h>
#include <mc_solver/ConstraintSetLoader.h>
#include <SpaceVecAlg/PTransform.h>

void AttachKnee::start(mc_control::fsm::Controller & ctl)
{
  output("OK");
  if(config_("display", true))
  {
    auto & kneeBraceRobot = ctl.loadRobot(mc_rbdyn::RobotLoader::get_robot_module("BoneTag::KneeBrace"), "knee_brace");
    auto & tibiaRobot = ctl.robot("panda_tibia");
    if(kneeBraceRobot.hasFrame("KneeBraceCenter") && tibiaRobot.hasFrame("Tibia"))
    {
      kneeBraceRobot.posW(kneeBraceRobot.frame("KneeBraceCenter").X_b_f().inv()
                          * ctl.robot("panda_tibia").frame("Tibia").position());
    }
  }
  for(const auto & col : config_("collisions"))
  {
    // ctl.addCollisions(col("r1"), col("r2"), col("collisions"), true);
    ctl.addCollisions(col("r1"), col("r2"), col("collisions"));
  }
}

bool AttachKnee::run(mc_control::fsm::Controller & ctl)
{
  return true;
}

void AttachKnee::teardown(mc_control::fsm::Controller & ctl) {}

EXPORT_SINGLE_STATE("AttachKnee", AttachKnee)
