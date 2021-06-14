cmake_minimum_required(VERSION 3.0)
project(DeviceSetup LANGUAGES CXX)

include(${AVS_CORE}/build/BuildDefaults.cmake)

add_subdirectory("interface")
add_subdirectory("capabilityagent")
