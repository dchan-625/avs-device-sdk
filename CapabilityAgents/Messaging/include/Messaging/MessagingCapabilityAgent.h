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

#ifndef ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MESSAGING_INCLUDE_MESSAGING_MESSAGINGCAPABILITYAGENT_H_
#define ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MESSAGING_INCLUDE_MESSAGING_MESSAGINGCAPABILITYAGENT_H_

#include <memory>
#include <unordered_set>
#include <unordered_map>

#include <AVSCommon/AVS/CapabilityAgent.h>
#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/SDKInterfaces/CapabilityConfigurationInterface.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/SDKInterfaces/ExceptionEncounteredSenderInterface.h>
#include <AVSCommon/SDKInterfaces/MessageSenderInterface.h>
#include <AVSCommon/SDKInterfaces/Messaging/MessagingObserverInterface.h>
#include <AVSCommon/SDKInterfaces/StateProviderInterface.h>
#include <AVSCommon/Utils/RequiresShutdown.h>
#include <AVSCommon/Utils/Threading/Executor.h>
#include <AVSCommon/Utils/UUIDGeneration/UUIDGeneration.h>

namespace alexaClientSDK {
namespace capabilityAgents {
namespace messaging {

/**
 * This class implements the @c MessagingController capability agent.
 *
 * @see https://developer.amazon.com/docs/alexa-voice-service/
 *
 * @note For instances of this class to be cleaned up correctly, @c shutdown() must be called.
 * @note This class makes use of a global configuration to a database file, meaning that it is best used
 * as a singleton.
 */
class MessagingCapabilityAgent
        : public avsCommon::avs::CapabilityAgent
        , public avsCommon::sdkInterfaces::CapabilityConfigurationInterface
        , public avsCommon::utils::RequiresShutdown
        , public std::enable_shared_from_this<MessagingCapabilityAgent> {
public:
    /**
     * Messaging endpoint reference.
     */
    using MessagingEndpoint = avsCommon::sdkInterfaces::messaging::MessagingObserverInterface::MessagingEndpoint;

    /**
     * An enum representing the status error codes.
     */
    enum class StatusErrorCode {
        /// Generic failure occurred during request processing.
        GENERIC_FAILURE,

        /// No connection was found during request processing.
        NO_CONNECTIVITY,

        /// Messaging endpoint does not have needed permissions.
        NO_PERMISSION
    };

    /**
     * An enum representing the @c UploadMode.
     */
    enum class UploadMode {
        /// Existing messages should be deleted and replaced with uploaded ones.
        DELETE_ALL_AND_STORE
    };

    /**
     * An enum representing the @c ConnectionState.
     */
    enum class ConnectionState {
        /// Messaging endpoint is disconnected.
        DISCONNECTED,

        /// Messaging endpoint is connected.
        CONNECTED
    };

    /**
     * An enum representing the @c Permission for @sa ConversationsReport.
     */
    enum class PermissionState {
        /// Permission is turned off.
        OFF,

        /// Permission is turned on.
        ON
    };

    /*
     *  Defines a container for the messaging endpoint state.
     */
    struct MessagingEndpointState {
        /*
         * Constructor. Initializes the configuration to default.
         */
        MessagingEndpointState() :
                connection{ConnectionState::DISCONNECTED},
                sendPermission{PermissionState::OFF},
                readPermission{PermissionState::OFF} {};

        /**
         * Constructor for initializing with specified states.
         * @param connectionIn The state of the connection.
         * @param sendPermission The state of the send permission.
         * @param readPermission The state of the read permission.
         */
        MessagingEndpointState(
            ConnectionState connectionIn,
            PermissionState sendPermissionIn,
            PermissionState readPermissionIn) :
                connection{connectionIn}, sendPermission{sendPermissionIn}, readPermission{readPermissionIn} {
        }

        /// Connection state
        ConnectionState connection;

        /// Send permission state.
        PermissionState sendPermission;

        /// Read permission state.
        PermissionState readPermission;
    };

    /**
     * Destructor.
     */
    virtual ~MessagingCapabilityAgent() = default;

    /**
     * Factory method to create a @c MessagingCapabilityAgent instance.
     *
     * @param exceptionSender Interface to report exceptions to AVS.
     * @param contextManager Interface to provide messaging state to AVS.
     * @param messageSender Interface to send events to AVS.
     * @return A new instance of @c MessagingCapabilityAgent on success, @c nullptr otherwise.
     */
    static std::shared_ptr<MessagingCapabilityAgent> create(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /// @name CapabilityAgent Functions
    /// @{
    alexaClientSDK::avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const override;
    void handleDirectiveImmediately(std::shared_ptr<alexaClientSDK::avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(
        std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void handleDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    void cancelDirective(std::shared_ptr<alexaClientSDK::avsCommon::avs::CapabilityAgent::DirectiveInfo> info) override;
    /// @}

    /// @name CapabilityConfigurationInterface method
    /// @{
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> getCapabilityConfigurations() override;
    /// @}

    /// @name RequiresShutdown method
    /// @{
    void doShutdown() override;
    /// @}

    /// @name StateProviderInterface Functions
    /// @{
    void provideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken) override;
    /// @}

    /**
     * Adds an observer to @c MessagingCapabilityAgent so that it will get notified for all
     * messaging related directives.
     *
     * @param observer The @c MessagingObserverInterface to add.
     */
    void addObserver(std::shared_ptr<avsCommon::sdkInterfaces::messaging::MessagingObserverInterface> observer);

    /**
     * Removes an observer from @c MessagingCapabilityAgent so that it will no longer be
     * notified of messaging related directives.
     *
     * @param observer The @c MessagingObserverInterface
     */
    void removeObserver(std::shared_ptr<avsCommon::sdkInterfaces::messaging::MessagingObserverInterface> observer);

    /**
     * Sends an event to notify AVS of new messages available on the device.
     *
     * @note This method should be called when the device is connected.
     * @note If this is called in response to an @c UploadConversations directive, then the token
     * received in @c UploadConversations should be passed back. Otherwise, an empty string should be sent.
     *
     * @param The token received from @c uploadConversations, otherwise an empty string.
     * @param conversations A JSON array representing the unread messages of the messaging endpoint.
     * @code{.json}
     * [
     *     {
     *         "id": "{{STRING}}",
     *         "otherParticipants": [
     *             {
     *                 "address":"{{STRING}}",
     *                 "addressType":"PhoneNumberAddress"
     *             }
     *         ],
     *         "messages": [
     *             {
     *                 "id":"{{STRING}}",
     *                 "payload": {
     *                     "@type":"text",
     *                     "text":"{{STRING}}"
     *                 },
     *                 "status":"unread",
     *                 "createdTime":"{{STRING}}",
     *                 "sender": {
     *                     "address":"{{STRING}}",
     *                     "addressType":"PhoneNumberAddress"
     *                 }
     *             }
     *         ],
     *         "unreadMessageCount":1
     *     }
     * ]
     * @endcode
     * @li id A unique identifier generated by the application for the conversation.
     * @li otherParticipants Optional recipients if messages are part of a group conversation, otherwise empty JSON
     * array.
     * @li otherParticipants.address The phone number of the recipient.
     * @li otherParticipants.addressType Hard coded string "PhoneNumberAddress" indicating the value of the @c address
     * field.
     * @li messages.id A unique identifier generated by the application for the message.
     * @li messages.payload.@type Hard coded string "text" indicating the value of the @c text field.
     * @li messages.text The text for the message.
     * @li messages.createdTime (optional) The ISO 8601 timestamp of when the message was created on the device.
     * @li messages.sender.address The phone number of the sender.
     * @li messages.sender.addressType Hard coded string "PhoneNumberAddress" indicating the value of the @c address
     * field.
     * @li unreadMessageCount The total number of unread messages in this conversation..
     */
    void conversationsReport(
        const std::string& token,
        const std::string& conversations,
        UploadMode mode = UploadMode::DELETE_ALL_AND_STORE,
        MessagingEndpoint messagingEndpoint = MessagingEndpoint::DEFAULT);

    /**
     * Sends an event to notify AVS that the message was sent successfully.
     *
     * @param token The token corresponding to the @c MessagingObserverInterface::sendMessage request.
     * @param messagingEndpoint The messaging endpoint that sent the message successfully.
     */
    void sendMessageSucceeded(
        const std::string& token,
        MessagingEndpoint messagingEndpoint = MessagingEndpoint::DEFAULT);

    /**
     * Sends an event to notify AVS that the message failed to be sent.
     *
     * @param token The token corresponding to the @c MessagingObserverInterface::sendMessage request.
     * @param code The @c StatusErrorCode describing why the request failed.
     * @param message The reason for the failure or empty string.
     * @param messagingEndpoint The messaging endpoint that failed to send the message.
     */
    void sendMessageFailed(
        const std::string& token,
        StatusErrorCode code,
        const std::string& message,
        MessagingEndpoint messagingEndpoint = MessagingEndpoint::DEFAULT);

    /**
     * Sends an event to notify AVS that the message status request was successful.
     *
     * @param token The token corresponding to the @c MessagingObserverInterface::updateMessagesStatus request.
     * @param messagingEndpoint The messaging endpoint that update status successfully.
     */
    void updateMessagesStatusSucceeded(
        const std::string& token,
        MessagingEndpoint messagingEndpoint = MessagingEndpoint::DEFAULT);

    /**
     * Sends an event to notify AVS that the message status request failed.
     *
     * @param token The token corresponding to the @c MessagingObserverInterface::sendMessage request.
     * @param code The @c StatusErrorCode describing why the request failed.
     * @param message The reason for the failure or empty string.
     * @param messagingEndpoint The messaging endpoint that failed to update status.
     */
    void updateMessagesStatusFailed(
        const std::string& token,
        StatusErrorCode code,
        const std::string& message,
        MessagingEndpoint messagingEndpoint = MessagingEndpoint::DEFAULT);

    /**
     * This function updates the @c MessagingCapabilityAgent context.
     *
     * @param messagingEndpointState The current state of the messaging endpoint.
     * @param messagingEndpoint The messaging endpoint whose state will be updated.
     */
    void updateMessagingEndpointState(
        MessagingEndpointState messagingEndpointState,
        MessagingEndpoint messagingEndpoint = MessagingEndpoint::DEFAULT);

private:
    /**
     * Constructor.
     *
     * @param exceptionSender Interface to report exceptions to AVS.
     * @param contextManager Interface to provide state to AVS.
     * @param messageSender Interface to send events to AVS
     */
    MessagingCapabilityAgent(
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::ContextManagerInterface> contextManager,
        std::shared_ptr<alexaClientSDK::avsCommon::sdkInterfaces::MessageSenderInterface> messageSender);

    /**
     * Initializes the object.
     */
    bool initialize();

    /**
     * Prepares MessagingController Interface DCF configuration and keeps it internally.
     */
    void generateCapabilityConfiguration();

    /**
     * Builds JSON string for the device capabilities reported.
     */
    std::string buildMessagingEndpointConfigurationJson();

    /**
     * Remove a directive from the map of message IDs to DirectiveInfo instances.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective whose message ID is to be removed.
     */
    void removeDirective(std::shared_ptr<DirectiveInfo> info);

    /**
     * Marks the directive as completed.
     *
     * @param info The directive currently being handled.
     */
    void executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info);

    /**
     * Handles the @c SendMessage AVS Directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     * @return @c true if operation succeeds and could be reported as such to AVS, @c false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool executeSendMessageDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * Handles the @c UpdateMessagesStatus AVS Directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload.
     * @return @c true if operation succeeds and could be reported as such to AVS, @c false if an error occurred. False
     * implies that exception has been reported to AVS and directive is already processed.
     */
    bool executeUpdateMessagesStatusDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * Handles the @c UploadConversations AVS Directive.
     *
     * @param info The @c DirectiveInfo containing the @c AVSDirective and the @c DirectiveHandlerResultInterface.
     * @param payload A @c rapidjson::Document holding the parsed directive payload*
     */
    bool executeUploadConversationsDirective(std::shared_ptr<DirectiveInfo> info, rapidjson::Document& payload);

    /**
     * This function updates the context for the @c MessagingCapabilityAgent.
     */
    void executeUpdateMessagingEndpointContext();

    /**
     * Gets the current state of the messaging endpoint and notifies @c ContextManager
     *
     * @param stateProviderName Provides the property name and used in the @c ContextManager methods.
     * @param contextRequestToken The token to be used when providing the response to @c ContextManager
     */
    void executeProvideState(
        const avsCommon::avs::CapabilityTag& stateProviderName,
        const avsCommon::sdkInterfaces::ContextRequestToken contextRequestToken);

    /// The ContextManager object.
    std::shared_ptr<avsCommon::sdkInterfaces::ContextManagerInterface> m_contextManager;

    /// The regular MessageSender object.
    std::shared_ptr<avsCommon::sdkInterfaces::MessageSenderInterface> m_messageSender;

    /// Set of capability configurations that will get published using DCF
    std::unordered_set<std::shared_ptr<avsCommon::avs::CapabilityConfiguration>> m_capabilityConfigurations;

    /// Set of observers of MessagingObserverInterface.
    std::unordered_set<std::shared_ptr<avsCommon::sdkInterfaces::messaging::MessagingObserverInterface>> m_observers;

    /// Map of messaging endpoint to state
    std::unordered_map<std::string, MessagingEndpointState> m_messagingEndpointsState;

    /// The current context for the messaging capability agent
    std::string m_messagingContext;

    /// Mutex to guard access of m_observers.
    std::mutex m_observerMutex;

    /// An executor used for serializing requests.
    avsCommon::utils::threading::Executor m_executor;
};

/**
 * Converts an enum to a string.
 *
 * @param messagingEndpoint The @c MessagingEndpoint value.
 * @return The string form of the enum.
 */
inline std::string messagingEndpointToString(MessagingCapabilityAgent::MessagingEndpoint messagingEndpoint) {
    switch (messagingEndpoint) {
        case MessagingCapabilityAgent::MessagingEndpoint::DEFAULT:
            return "DEFAULT";
    }

    return "UNKNOWN";
}

/**
 * Converts an enum to a string.
 *
 * @param code The @c StatusErrorCode.
 * @return The string form of the enum.
 */
inline std::string statusErrorCodeToString(MessagingCapabilityAgent::StatusErrorCode code) {
    switch (code) {
        case MessagingCapabilityAgent::StatusErrorCode::GENERIC_FAILURE:
            return "GENERIC_FAILURE";
        case MessagingCapabilityAgent::StatusErrorCode::NO_CONNECTIVITY:
            return "NO_CONNECTIVITY";
        case MessagingCapabilityAgent::StatusErrorCode::NO_PERMISSION:
            return "NO_PERMISSION";
    }

    return "UNKNOWN";
}

/**
 * Converts an enum to a string.
 *
 * @param code The @c UploadMode.
 * @return The string form of the enum.
 */
inline std::string uploadModeToString(MessagingCapabilityAgent::UploadMode mode) {
    switch (mode) {
        case MessagingCapabilityAgent::UploadMode::DELETE_ALL_AND_STORE:
            return "DELETE_ALL_AND_STORE";
    }

    return "UNKNOWN";
}

/**
 * Converts an enum to a string.
 *
 * @param code The @c ConnectionState.
 * @return The string form of the enum.
 */
inline std::string connectionStateToString(MessagingCapabilityAgent::ConnectionState connection) {
    switch (connection) {
        case MessagingCapabilityAgent::ConnectionState::CONNECTED:
            return "CONNECTED";
        case MessagingCapabilityAgent::ConnectionState::DISCONNECTED:
            return "DISCONNECTED";
    }

    return "UNKNOWN";
}

/**
 * Converts an enum to a string.
 *
 * @param code The @c PermissionState.
 * @return The string form of the enum.
 */
inline std::string permissionStateToString(MessagingCapabilityAgent::PermissionState permission) {
    switch (permission) {
        case MessagingCapabilityAgent::PermissionState::ON:
            return "ON";
        case MessagingCapabilityAgent::PermissionState::OFF:
            return "OFF";
    }

    return "UNKNOWN";
}

}  // namespace messaging
}  // namespace capabilityAgents
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_CAPABILITYAGENTS_MESSAGING_INCLUDE_MESSAGING_MESSAGINGCAPABILITYAGENT_H_