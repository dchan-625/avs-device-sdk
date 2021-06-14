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

#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGING_MESSAGINGOBSERVERINTERFACE_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGING_MESSAGINGOBSERVERINTERFACE_H_

#include <string>

namespace alexaClientSDK {
namespace avsCommon {
namespace sdkInterfaces {
namespace messaging {

/**
 * This @c MessagingObserverInterface class is used to notify observers when a @c SendMessage,
 * @c UpdateMessageStatus, or @c UploadConversations directive is received.
 */
class MessagingObserverInterface {
public:
    /**
     * An enum representing the messaging endpoints.
     */
    enum class MessagingEndpoint {
        /// Default messaging endpoint
        DEFAULT
    };

    /**
     * Destructor
     */
    virtual ~MessagingObserverInterface() = default;

    /**
     * Used to notify the observer when a @c Alexa.Comms.MessagingController.SendMessage directive
     * is received. Once called, the client should send the message to the given recipients
     * using the specified messaging endpoint.
     *
     * @param token The token for this request.
     * @param endpoint The messaging endpoint to send the message.
     * @param jsonPayload The payload of the @c SendMessage directive in structured JSON format.
     * @code{.json}
     * {
     *     "payload": {
     *         "@type" : "text",
     *         "text" : "{{STRING}}"
     *     },
     *     "recipients" : [
     *         {
     *             "address" : "{{STRING}}",
     *             "addressType" : "PhoneNumberAddress"
     *         }
     *     ]
     * }
     * @endcode
     *
     */
    virtual void sendMessage(const std::string& token, MessagingEndpoint endpoint, const std::string& jsonPayload) = 0;

    /**
     * Used to notify the observer when a @c Alexa.Comms.MessagingController.UploadConversations directive
     * is received. Once called, the client should upload a conversations report using the specified
     * filter in the JSON payload.
     *
     * @param token The token for this request.
     * @param endpoint The messaging endpoint whose messages are requested to be uploaded.
     * @param jsonPayload The payload of the @c UploadConversations directive in structured JSON format. The only
     * supported filter values are shown below.
     * @code{.json}
     * {
     *     "filter" : {
     *         "conversationFilter" : {
     *             "@type" : "AllConversations",
     *             "conversationIds" : [{{STRING}}]
     *         },
     *         "messageFilter" : {
     *             "@type" : "UnreadMessages"
     *         },
     *         "maxMessageCount" : 40
     *      }
     * }
     * @endcode
     *
     */
    virtual void uploadConversations(
        const std::string& token,
        MessagingEndpoint endpoint,
        const std::string& jsonPayload) = 0;

    /**
     * Used to notify the observer when a @c Alexa.Comms.MessagingController.UpdateMessagesStatus directive
     * is received. Once called, the client should update the specified messages with the given
     * status.
     *
     * @param token The token for this request.
     * @param endpoint The messaging endpoint whose messages status need to be updated.
     * @param jsonPayload The payload of the @c UpdateMessagesStatus directive in structured JSON format.
     * @code{.json}
     * {
     *     "conversationId" : {{STRING}},
     *     "statusMap" : {
     *         "read" : [{{STRING}}],
     *     }
     * }
     * @endcode
     */
    virtual void updateMessagesStatus(
        const std::string& token,
        MessagingEndpoint endpoint,
        const std::string& jsonPayload) = 0;
};

}  // namespace messaging
}  // namespace sdkInterfaces
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_AVSCOMMON_SDKINTERFACES_INCLUDE_AVSCOMMON_SDKINTERFACES_MESSAGING_MESSAGINGOBSERVERINTERFACE_H_