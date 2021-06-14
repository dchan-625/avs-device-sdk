add_definitions("-DACSDK_LOG_MODULE=messaging")

add_library(Messaging SHARED
        MessagingCapabilityAgent.cpp)

target_include_directories(Messaging PUBLIC
        "${Messaging_SOURCE_DIR}/include"
        "${AVSCommon_INCLUDE_DIRS}")

target_link_libraries(Messaging AVSCommon)

# install target
asdk_install()
