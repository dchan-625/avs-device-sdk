LOCAL_PATH:= $(call my-dir)

#########################
# Declare common variables

DeviceSetup_SRC_FILES := \
    $(call all-cpp-files-under,src/)
DeviceSetup_TEST_FILES := \
    $(call all-cpp-files-under,test/)
DeviceSetup_C_INCLUDES := $(LOCAL_PATH)/include
DeviceSetup_CFLAGS := \
    $(AVS_COMMON_CFLAGS) \
    -DACSDK_LOG_MODULE

#########################
# Build the shared library
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(DeviceSetup_SRC_FILES)
LOCAL_C_INCLUDES := $(DeviceSetup_C_INCLUDES)
LOCAL_CFLAGS := $(DeviceSetup_CFLAGS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(DeviceSetup_C_INCLUDES)

LOCAL_MODULE := libDeviceSetupCA
LOCAL_SHARED_LIBRARIES := libAVSCommon libDeviceSetupInterfaces

include $(BUILD_SHARED_LIBRARY)

#########################
# Build the shared host library
include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(DeviceSetup_SRC_FILES)
LOCAL_C_INCLUDES := $(DeviceSetup_C_INCLUDES)
LOCAL_CFLAGS := $(DeviceSetup_CFLAGS)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(DeviceSetup_C_INCLUDES)

LOCAL_MODULE := libDeviceSetupCA-host
LOCAL_SHARED_LIBRARIES := libAVSCommon-host libDeviceSetupInterfaces-host
LOCAL_MULTILIB := 64

include $(BUILD_HOST_SHARED_LIBRARY)