cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

set(INCLUDE_PATH
    "${DeviceSetupCA_INCLUDE_DIRS}"
    "${AVSCommon_SOURCE_DIR}/AVS/test")

set(LIBS
        "DeviceSetupCA"
        "SDKInterfacesTests")

discover_unit_tests("${INCLUDE_PATH}" "${LIBS}")
