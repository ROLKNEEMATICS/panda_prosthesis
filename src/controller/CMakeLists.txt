set(common_SRC
  src/Trajectory.cpp
  src/TrajectoryPlayer.cpp
  src/PandaProsthesisImpedanceTask.cpp
)
set(common_HDR
  include/Trajectory.h
  include/TrajectoryPlayer.h
  include/utils.h
  include/PandaProsthesisImpedanceTask.h
)
add_library(PandaProsthesisCommon SHARED ${common_SRC} ${common_HDR})
target_include_directories(PandaProsthesisCommon PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
target_link_libraries(PandaProsthesisCommon PUBLIC SpaceVecAlg::SpaceVecAlg mc_rtc::mc_control)
install(TARGETS PandaProsthesisCommon
  LIBRARY DESTINATION lib)

# PandaProsthesis controller (BoneTag)
add_controller(PandaProsthesisController PandaProsthesisController.cpp PandaProsthesisController.h)
target_link_libraries(PandaProsthesisController PUBLIC PandaProsthesisCommon)
target_link_libraries(PandaProsthesisController PUBLIC mc_rtc::mc_control_fsm)
target_link_libraries(PandaProsthesisController PUBLIC mc_panda::devices)
target_include_directories(PandaProsthesisController PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../module/include>)

install(DIRECTORY "${PROJECT_SOURCE_DIR}/data/trajectories"
        DESTINATION share/${PROJECT_NAME}/data)
set(TRAJECTORY_DIR "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/data/trajectories")

set(CALIB_DIR_INIT "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/data/calibration")
install(DIRECTORY etc/bonetag_panda DESTINATION ${CALIB_DIR_INIT} FILES_MATCHING PATTERN "*.yaml")
# install(FILES ${PROJECT_SOURCE_DIR}/etc/bonetag_panda/*.yaml DESTINATION "share/${PROJECT_NAME}/calibration/bonetag_panda")
# install(FILES ${PROJECT_SOURCE_DIR}/etc/bonetag_ur10/*.yaml DESTINATION "share/${PROJECT_NAME}/calibration/bonetag_ur10")

# set(RESULTS_DIR "${PROJECT_SOURCE_DIR}/data/results")
configure_file(etc/PandaProsthesis.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/etc/PandaProsthesis.yaml")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/etc/PandaProsthesis.yaml" DESTINATION "${MC_CONTROLLER_RUNTIME_INSTALL_PREFIX}/etc")
configure_file(etc/mc_rtc.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/etc/mc_rtc.yaml")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/etc/mc_rtc.yaml" DESTINATION "${MC_CONTROLLER_RUNTIME_INSTALL_PREFIX}/etc/${PROJECT_NAME}/")


# PandaBrace controller (Lea's orthesis)
# TODO(nix): use get_or_create_results_dir() instead of RESULTS_DIR variable
if(TARGET PandaBraceModule)
  message(STATUS "Will build PandaBraceController as the PandaBraceModule is available")
  add_controller(PandaBraceController PandaBraceController.cpp PandaBraceController.h)
  target_link_libraries(PandaBraceController PUBLIC PandaProsthesisCommon)
  target_link_libraries(PandaBraceController PUBLIC mc_rtc::mc_control_fsm)
  target_link_libraries(PandaBraceController PUBLIC mc_panda::devices)
  target_include_directories(PandaBraceController PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/../module/include>)

  set(RESULTS_DIR "${PROJECT_SOURCE_DIR}/data/results")
  configure_file(etc/PandaBrace.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/etc/PandaBrace.yaml")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/etc/PandaBrace.yaml" DESTINATION "${MC_CONTROLLER_RUNTIME_INSTALL_PREFIX}/etc")

  configure_file(etc/AtiDaq.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/etc/AtiDaq.yaml")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/etc/AtiDaq.yaml" DESTINATION "${MC_CONTROLLER_RUNTIME_INSTALL_PREFIX}/etc/PandaBrace/plugins/")
  install(FILES etc/PhidgetPressureSensorPlugin.yaml DESTINATION "${MC_CONTROLLER_RUNTIME_INSTALL_PREFIX}/etc/PandaBrace/plugins/")
endif()



add_subdirectory(states)
