/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/apache2.0/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#ifndef ALEXA_CLIENT_SDK_AVS_CAPABILITIES_DEVICESETUP_CAPABILITYAGENT_INCLUDE_DEVICESETUP_DEVICESETUP_H_
#define ALEXA_CLIENT_SDK_AVS_CAPABILITIES_DEVICESETUP_CAPABILITYAGENT_INCLUDE_DEVICESETUP_DEVICESETUP_H_

#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <DeviceSetup/Interfaces/DeviceSetupInterface.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace deviceSetup {

/**
 * The @c DeviceSetup capability agent. The purpose of this CA is to noitfy the cloud when DeviceSetup has completed.
 * DeviceSetupInterface::sendDeviceSetupComplete will return a future.
 */
class DeviceSetup
        : public interfaces::deviceSetup::DeviceSetupInterface
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public std::enable_shared_from_this<DeviceSetup> {
public:
    /**
     * Create an instance of the DeviceSetup CA.
     *
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     * @return A ptr to the DeviceSetup CA if successful or nullptr otherwise.
     */
    static std::shared_ptr<DeviceSetup> create(
        std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// @name CapabilityConfigurationInterface Functions
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name DeviceSetupInterface Functions
    /// @{
    std::future<bool> sendDeviceSetupComplete(interfaces::deviceSetup::AssistedSetup assistedSetup) override;
    /// @}

private:
    /**
     * Constructor.
     *
     * @param messageSender A @c MessageSenderInterface to send messages to AVS.
     */
    DeviceSetup(std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// The @c MessageSenderInterface used to send event messages.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;
};

}  // namespace deviceSetup
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVS_CAPABILITIES_DEVICESETUP_CAPABILITYAGENT_INCLUDE_DEVICESETUP_DEVICESETUP_H_
