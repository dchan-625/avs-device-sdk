add_definitions("-DACSDK_LOG_MODULE=deviceSetup")

add_library(
    DeviceSetupCA SHARED
    DeviceSetup.cpp
    DeviceSetupMessageRequest.cpp
)

target_include_directories(DeviceSetupCA PUBLIC
    "${DeviceSetupCA_SOURCE_DIR}/include")

target_link_libraries(
    DeviceSetupCA
    DeviceSetupInterface
    AVSCommon
)

# install target
asdk_install()
