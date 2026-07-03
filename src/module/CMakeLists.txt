set(MODULE_INSTALL_LOCATION "${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}")

configure_file(config.h.in "${CMAKE_CURRENT_BINARY_DIR}/include/config.h")

macro(add_yaml_module NAME)
  configure_file(urdf/${NAME}.in.urdf "${CMAKE_CURRENT_BINARY_DIR}/urdf/${NAME}.urdf")
  configure_file(yaml/${NAME}.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/yaml/${NAME}.yaml")
# Configure the alias accordingly
  configure_file(alias/${NAME}.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/alias/${NAME}.yaml")
# Install the alias
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/alias/${NAME}.yaml" DESTINATION "${MC_ROBOTS_ALIASES_DIRECTORY}")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/urdf/${NAME}.urdf" DESTINATION "${MODULE_INSTALL_LOCATION}/urdf")
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/yaml/${NAME}.yaml" DESTINATION "${MODULE_INSTALL_LOCATION}/yaml")
  install(DIRECTORY rsdf/${NAME} DESTINATION "${MODULE_INSTALL_LOCATION}/rsdf" FILES_MATCHING PATTERN "*.rsdf")
  install(DIRECTORY convex/${NAME} DESTINATION "${MODULE_INSTALL_LOCATION}/convex" FILES_MATCHING PATTERN "*.txt")
  install(DIRECTORY meshes/${NAME} DESTINATION "${MODULE_INSTALL_LOCATION}/meshes"
    FILES_MATCHING PATTERN "*.stl" PATTERN "*.STL" PATTERN "*.dae" PATTERN "*.DAE"
  )
endmacro()
add_yaml_module(bonetag_tibia)
add_yaml_module(bonetag_tibia_calibration_tool)
add_yaml_module(bonetag_femur)
add_yaml_module(bonetag_knee_brace)
add_yaml_module(taa_talar)
add_yaml_module(taa_tibia)
add_yaml_module(tsa_glenoid)
add_yaml_module(tsa_humerus)
add_yaml_module(tka_femur)
add_yaml_module(tka_tibia)
add_yaml_module(tha_femur)
add_yaml_module(tha_acetabular)
add_yaml_module(bloc_support)



add_robot(PandaProsthesisModule module.cpp module.h)
target_link_libraries(PandaProsthesisModule PUBLIC mc_panda::panda)
target_include_directories(PandaProsthesisModule PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

# XXX: The OPTIONAL argument should be part of mc_rtc's find_description_package
macro(find_description_package_optional PACKAGE)
  set(PACKAGE_PATH_VAR "${PACKAGE}_PATH")
  string(TOUPPER "${PACKAGE_PATH_VAR}" PACKAGE_PATH_VAR)
  find_package(${PACKAGE} QUIET)
  if(${PACKAGE}_FOUND)
    find_description_package(${PACKAGE})
  endif()
endmacro()

find_description_package_optional(brace_bottom_setup)
if(brace_bottom_setup_FOUND)
  set(brace_bottom_setup_DIR ${BRACE_BOTTOM_SETUP_PATH})
  message(STATUS "Found brace_bottom_setup sources in ${brace_bottom_setup_DIR}")

  # Generate the robot module YAML description using this value
  configure_file(yaml/brace_bottom_setup.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/yaml/brace_bottom_setup.yaml")

  # Install the YAML file
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/yaml/brace_bottom_setup.yaml" DESTINATION "${MODULE_INSTALL_LOCATION}/yaml")

  # Configure the alias accordingly
  configure_file(alias/brace_bottom_setup.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/alias/brace_bottom_setup.yaml")

  # Install the alias
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/alias/brace_bottom_setup.yaml" DESTINATION "${MC_ROBOTS_ALIASES_DIRECTORY}")
endif()

find_description_package_optional(brace_urdf)
if(brace_urdf_FOUND)
  set(brace_urdf_DIR ${BRACE_URDF_PATH})
  message(STATUS "Found brace_urdf sources in ${brace_urdf_DIR}")

  # Generate the robot module YAML description using this value
  configure_file(yaml/brace_urdf.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/yaml/brace_urdf.yaml")

  # Install the YAML file
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/yaml/brace_urdf.yaml" DESTINATION "${MODULE_INSTALL_LOCATION}/yaml")

  # Configure the alias accordingly
  configure_file(alias/brace_urdf.in.yaml "${CMAKE_CURRENT_BINARY_DIR}/alias/brace_urdf.yaml")

  # Install the alias
  install(FILES "${CMAKE_CURRENT_BINARY_DIR}/alias/brace_urdf.yaml" DESTINATION "${MC_ROBOTS_ALIASES_DIRECTORY}")
endif()

find_description_package_optional(brace_top_setup)
if(brace_top_setup_FOUND)
  set(brace_top_setup_DIR ${BRACE_TOP_SETUP_PATH})
  message(STATUS "Found brace_top_setup sources in ${brace_top_setup_DIR}")

  configure_file(alias/brace_top_setup.in.yaml ${CMAKE_CURRENT_BINARY_DIR}/alias/brace_top_setup.yaml)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/alias/brace_top_setup.yaml DESTINATION ${MC_ROBOTS_ALIASES_DIRECTORY})
else()
  message(WARNING "brace_top_setup not found")
endif()

find_description_package_optional(brace_top_setup_brace)
if(brace_top_setup_brace_FOUND)
  set(brace_top_setup_brace_DIR ${BRACE_TOP_SETUP_BRACE_PATH})
  message(STATUS "Found brace_top_setup_brace sources in ${brace_top_setup_brace_DIR}")

  configure_file(alias/brace_top_setup_brace.in.yaml ${CMAKE_CURRENT_BINARY_DIR}/alias/brace_top_setup_brace.yaml)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/alias/brace_top_setup_brace.yaml DESTINATION ${MC_ROBOTS_ALIASES_DIRECTORY})
else()
  message(WARNING "brace_top_setup_brace not found")
endif()

if(brace_top_setup_FOUND AND brace_top_setup_brace_FOUND)
  message(STATUS "Found both brace_top_setup and brace_top_setup_brace, building PandaBrace robot module")
  configure_file(config_panda_brace.h.in "${CMAKE_CURRENT_BINARY_DIR}/include/config_panda_brace.h")
  add_robot(PandaBraceModule panda_brace_module.cpp panda_brace_module.h)
  target_link_libraries(PandaBraceModule PUBLIC mc_panda::panda)
  target_include_directories(PandaBraceModule PRIVATE $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>)

  install(FILES etc/ForceSensorCalibration/panda_brace_femur.yaml DESTINATION ${MC_CONTROLLER_RUNTIME_INSTALL_PREFIX}/ForceSensorCalibration/)
endif()
