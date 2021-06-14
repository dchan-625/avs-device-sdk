LOCAL_PATH:= $(call my-dir)

DeviceSetup_INTERFACES := $(LOCAL_PATH)/include

#########################
# Build the shared library
include $(CLEAR_VARS)
LOCAL_C_INCLUDES := $(DeviceSetup_INTERFACES)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(DeviceSetup_INTERFACES)

LOCAL_MODULE := libDeviceSetupInterfaces
include $(BUILD_SHARED_LIBRARY)

#########################
# Build the shared host library
include $(CLEAR_VARS)

LOCAL_C_INCLUDES := $(DeviceSetup_INTERFACES)
LOCAL_EXPORT_C_INCLUDE_DIRS := $(DeviceSetup_INTERFACES)

LOCAL_MODULE := libDeviceSetupInterfaces-host
LOCAL_MULTILIB := 64

include $(BUILD_HOST_SHARED_LIBRARY)