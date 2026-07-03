#include "module.h"

#include <mc_rbdyn/RobotLoader.h>

extern "C"
{
  ROBOT_MODULE_API void MC_RTC_ROBOT_MODULE(std::vector<std::string> & names)
  {
    mc_robots::ForAllVariants([&names](const auto & robot, const auto & tool)
                              { names.push_back(robot.module + "::" + tool.module); });
  }
  ROBOT_MODULE_API void destroy(mc_rbdyn::RobotModule * ptr)
  {
    delete ptr;
  }
  ROBOT_MODULE_API mc_rbdyn::RobotModule * create(const std::string & n)
  {
    ROBOT_MODULE_CHECK_VERSION("PandaProsthesis")
    static auto variant_factory = []()
    {
      std::map<std::string, std::function<mc_rbdyn::RobotModule *()>> variant_factory;
      using namespace mc_robots;
      ForAllVariants(
          [&variant_factory](const auto & robot, const auto & tool)
          {
            auto variant_name = robot.module + "::" + tool.module;
            variant_factory[variant_name] = [=]()
            {
              auto name = robot.name + "_" + tool.name;
              auto robot_rm = *mc_rbdyn::RobotLoader::get_robot_module(robot.module);
              auto femur_rm = *mc_rbdyn::RobotLoader::get_robot_module(tool.module);
              auto connect_rm = new mc_rbdyn::RobotModule(robot_rm.connect(
                  femur_rm, robot.connection_link, tool.connection_link, "",
                  mc_rbdyn::RobotModule::ConnectionParameters{}.name(name).X_other_connection(sva::RotZ(tool.rotate))));
              connect_rm->name = name;

              const double i = 0.01;
              const double s = 0.005;
              const double d = 0.;
              auto & c = connect_rm->_minimalSelfCollisions;
              if(tool.module == "BoneTag::Femur" || tool.module == "BoneTag::Tibia")
              {
                c.push_back({tool.name, "robot_support", i, s, d});
                c.push_back({"support_" + tool.name, "robot_support", i, s, d});
              }

              return connect_rm;
            };
          });
      return variant_factory;
    }();

    auto it = variant_factory.find(n);
    if(it != variant_factory.end())
    {
      return it->second();
    }
    else
    {
      mc_rtc::log::error("HRP4CR module Cannot create an object of type {}", n);
      return nullptr;
    }
  }
}
