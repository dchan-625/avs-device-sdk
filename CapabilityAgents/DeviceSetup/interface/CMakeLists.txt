cmake_minimum_required(VERSION 3.0)
project(DeviceSetupInterface LANGUAGES CXX)

add_library(DeviceSetupInterface INTERFACE)

target_include_directories(DeviceSetupInterface INTERFACE "${DeviceSetupInterface_SOURCE_DIR}/include")

# install target
asdk_install_interface()
