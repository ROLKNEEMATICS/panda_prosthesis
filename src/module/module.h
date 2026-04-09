#pragma once

#include <mc_rbdyn/RobotModuleMacros.h>
#include <mc_robots/api.h>

namespace mc_robots
{

struct ConnectRobot
{
  std::string module;
  std::string name;
  std::string connection_link;
};

struct ConnectTool
{
  std::string module;
  std::string name;
  std::string connection_link;
  double rotate = 0;
};

template<typename Callback>
static void ForAllVariants(Callback cb)
{
  static std::string panda2 = "Panda2LIRMM_FR1_Default_Table2_Calibrated";
  static std::string panda7 = "Panda7LIRMM_FR1_Default_Table2_Calibrated";
  // cb(ConnectRobot{"Panda2LIRMM_2::Default", "panda", "panda_link8"},
  //    ConnectTool{"BoneTag::Femur", "femur", "femur_base_link"});
  // cb(ConnectRobot{"Panda7LIRMM_2::Default", "panda", "panda_link8"},
  //    ConnectTool{"BoneTag::Tibia", "tibia", "tibia_base_link"});
  cb(ConnectRobot{panda2, "panda", "panda_link8"},
     ConnectTool{"BoneTag::Femur", "femur", "femur_base_link"});
  cb(ConnectRobot{panda7, "panda", "panda_link8"},
     ConnectTool{"BoneTag::Tibia", "tibia", "tibia_base_link"});

  // creates panda_taa_tibia
  cb(ConnectRobot{panda2, "panda", "panda_link8"},
     ConnectTool{"TSA::Glenoid", "tsa_glenoid", "base_link"});
  // creates panda_tsa_humerus
  cb(ConnectRobot{panda7, "panda", "panda_link8"},
     ConnectTool{"TSA::Humerus", "tsa_humerus", "base_link"});
  for(auto & robot : std::vector<ConnectRobot>{{"UR10", "panda", "wrist_3_link"}})
  {
    for(auto & tool : std::vector<ConnectTool>{{"BoneTag::Femur", "femur", "femur_base_link"},
                                               {"BoneTag::Tibia", "tibia", "tibia_base_link"}})
    {
      cb(robot, tool);
    }
  }
}

} // namespace mc_robots
