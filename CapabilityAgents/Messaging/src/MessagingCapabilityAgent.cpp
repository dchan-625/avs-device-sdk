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

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include <AVSCommon/AVS/CapabilityConfiguration.h>
#include <AVSCommon/AVS/MessageRequest.h>
#include <AVSCommon/SDKInterfaces/ContextManagerInterface.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>
#include <AVSCommon/Utils/Logger/Logger.h>

#include "Messaging/MessagingCapabilityAgent.h"

namespace alexaClientSDK {
namespace capabilityAgents {
namespace messaging {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::sdkInterfaces::messaging;
using namespace avsCommon::utils::json::jsonUtils;
using namespace avsCommon::utils::json;
using namespace avsCommon::utils::logger;
using namespace rapidjson;

// ==== Messaging Capability Agent constants ===

/// String to identify log entries originating from this file.
static const std::string TAG{"Messaging"};

/// The MessagingController interface namespace.
static const std::string NAMESPACE{"Alexa.Comms.MessagingController"};

/// MessagingController interface type.
static const std::string MESSAGING_CAPABILITY_INTERFACE_TYPE = "AlexaInterface";

/// MessagingController interface version.
static const std::string MESSAGING_CAPABILITY_INTERFACE_VERSION = "1.0";

/// The MessagingController context namespace.
static const NamespaceAndName CONTEXT_MANAGER_MESSAGING_STATE{NAMESPACE, "MessagingControllerState"};

// ==== Directives ===

/// The @c SendMessage directive identifier.
static const NamespaceAndName SEND_MESSAGE{NAMESPACE, "SendMessage"};

/// The @c UpdateMessagesStatus directive identifier.
static const NamespaceAndName UPDATE_MESSAGES_STATUS{NAMESPACE, "UpdateMessagesStatus"};

/// The @c UploadConversations directive identifier.
static const NamespaceAndName UPLOAD_CONVERSATIONS{NAMESPACE, "UploadConversations"};

// ==== Events ===

/// The @c SendMessageSucceeded event identifier.
static const std::string SEND_MESSAGE_SUCCEEDED = "SendMessageSucceeded";

/// The @c SendMessageFailed event identifier.
static const std::string SEND_MESSAGE_FAILED = "SendMessageFailed";

/// The @c UpdateMessagesStatusSucceeded event identifier.
static const std::string UPDATE_MESSAGES_STATUS_SUCCEEDED = "UpdateMessagesStatusSucceeded";

/// The @c UpdateMessagesStatusFailed event identifier.
static const std::string UPDATE_MESSAGES_STATUS_FAILED = "UpdateMessagesStatusFailed";

/// The @c ConversationsReport event identifier.
static const std::string CONVERSATIONS_REPORT = "ConversationsReport";

// ==== JSON constants ===

/// Name for "token" JSON key.
static constexpr char JSON_KEY_TOKEN[] = "token";

/// Name for "conversationId" JSON key.
static constexpr char JSON_KEY_CONVERSATION_ID[] = "conversationId";

/// Name for "statusMap" JSON key.
static constexpr char JSON_KEY_STATUS_MAP[] = "statusMap";

/// Name for "messagingEndpoints" JSON key.
static constexpr char JSON_KEY_MESSAGING_ENDPOINTS[] = "messagingEndpoints";

/// Name for "messagingEndpointInfo" JSON key.
static constexpr char JSON_KEY_MESSAGING_ENDPOINT_INFO[] = "messagingEndpointInfo";

/// Name for "name" JSON key.
static constexpr char JSON_KEY_MESSAGING_ENDPOINT_NAME[] = "name";

/// Name for "messagingEndpointStates" JSON key.
static constexpr char JSON_KEY_MESSAGING_ENDPOINT_STATES[] = "messagingEndpointStates";

/// Name for "permissions" JSON key.
static constexpr char JSON_KEY_MESSAGING_PERMISSIONS[] = "permissions";

/// Name for "sendPermissions" JSON key.
static constexpr char JSON_KEY_MESSAGING_SEND_PERMISSION[] = "sendPermission";

/// Name for "readPermissions" JSON key.
static constexpr char JSON_KEY_MESSAGING_READ_PERMISSION[] = "readPermission";

/// Name for "connectionState" JSON key.
static constexpr char JSON_KEY_CONNECTION_STATE[] = "connectionState";

/// Name for "status" JSON value.
static constexpr char JSON_KEY_STATUS[] = "status";

/// Name for "uploadMode" JSON value.
static constexpr char JSON_KEY_UPLOAD_MODE[] = "uploadMode";

/// Name for "code" JSON key.
static constexpr char JSON_KEY_STATUS_CODE[] = "code";

/// Name for "message" JSON key.
static constexpr char JSON_KEY_STATUS_MESSAGE[] = "message";

/// Name for "conversations" JSON key.
static constexpr char JSON_KEY_CONVERSATIONS[] = "conversations";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

std::shared_ptr<MessagingCapabilityAgent> MessagingCapabilityAgent::create(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) {
    if (!contextManager) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullContextManager"));
        return nullptr;
    }

    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullExceptionSender"));
        return nullptr;
    }

    if (!messageSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullMessageSender"));
        return nullptr;
    }

    // Create instance of capability agent
    auto messagingCapabilityAgent = std::shared_ptr<MessagingCapabilityAgent>(
        new MessagingCapabilityAgent(exceptionSender, contextManager, messageSender));

    messagingCapabilityAgent->initialize();

    return messagingCapabilityAgent;
}

MessagingCapabilityAgent::MessagingCapabilityAgent(
    std::shared_ptr<ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<ContextManagerInterface> contextManager,
    std::shared_ptr<MessageSenderInterface> messageSender) :
        CapabilityAgent{NAMESPACE, exceptionSender},
        avsCommon::utils::RequiresShutdown{"MessagingCapabilityAgent"},
        m_contextManager{contextManager},
        m_messageSender{messageSender} {
}

bool MessagingCapabilityAgent::initialize() {
    ACSDK_INFO(LX(__func__));
    // Initialize endpoint state values. One per defined messaging endpoint.
    m_messagingEndpointsState.emplace(messagingEndpointToString(MessagingEndpoint::DEFAULT), MessagingEndpointState{});
    // Generate the device capability configuration
    generateCapabilityConfiguration();
    // Register with the context manager
    m_contextManager->addStateProvider(CONTEXT_MANAGER_MESSAGING_STATE, shared_from_this());
    // Initialize the context
    executeUpdateMessagingEndpointContext();

    return true;
}

void MessagingCapabilityAgent::generateCapabilityConfiguration() {
    std::unordered_map<std::string, std::string> configMap;

    configMap.insert({CAPABILITY_INTERFACE_TYPE_KEY, MESSAGING_CAPABILITY_INTERFACE_TYPE});
    configMap.insert({CAPABILITY_INTERFACE_NAME_KEY, NAMESPACE});
    configMap.insert({CAPABILITY_INTERFACE_VERSION_KEY, MESSAGING_CAPABILITY_INTERFACE_VERSION});
    configMap.insert({CAPABILITY_INTERFACE_CONFIGURATIONS_KEY, buildMessagingEndpointConfigurationJson()});

    m_capabilityConfigurations.insert(std::make_shared<CapabilityConfiguration>(configMap));
}

std::string MessagingCapabilityAgent::buildMessagingEndpointConfigurationJson() {
    avsCommon::utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.startArray(JSON_KEY_MESSAGING_ENDPOINTS);
    jsonGenerator.startArrayElement();
    jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
    jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, messagingEndpointToString(MessagingEndpoint::DEFAULT));
    jsonGenerator.finishObject();
    jsonGenerator.finishArrayElement();
    jsonGenerator.finishArray();

    ACSDK_DEBUG5(LX(__func__).sensitive("configuration", jsonGenerator.toString()));
    return jsonGenerator.toString();
}

DirectiveHandlerConfiguration MessagingCapabilityAgent::getConfiguration() const {
    DirectiveHandlerConfiguration configuration;
    auto noneNonBlockingPolicy = BlockingPolicy(BlockingPolicy::MEDIUMS_NONE, false);

    configuration[SEND_MESSAGE] = noneNonBlockingPolicy;
    configuration[UPDATE_MESSAGES_STATUS] = noneNonBlockingPolicy;
    configuration[UPLOAD_CONVERSATIONS] = noneNonBlockingPolicy;

    return configuration;
}

void MessagingCapabilityAgent::provideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG5(
        LX(__func__).d("contextRequestToken", contextRequestToken).sensitive("stateProviderName", stateProviderName));

    m_executor.submit([this, stateProviderName, contextRequestToken] {
        ACSDK_DEBUG5(LX("provideStateInExecutor"));
        executeProvideState(stateProviderName, contextRequestToken);
    });
    return;
}

void MessagingCapabilityAgent::handleDirectiveImmediately(std::shared_ptr<AVSDirective> directive) {
    ACSDK_DEBUG5(LX(__func__));
    handleDirective(std::make_shared<DirectiveInfo>(directive, nullptr));
}

void MessagingCapabilityAgent::preHandleDirective(std::shared_ptr<DirectiveInfo> info) {
    // No-op
}

/**
 * Parses a directive payload JSON and returns a parsed document object.
 *
 * @param payload JSON string to parse.
 * @param[out] document Pointer to a parsed document.
 * @return True if parsing was successful, false otherwise.
 */
static bool parseDirectivePayload(const std::string& payload, Document* document) {
    ACSDK_DEBUG5(LX(__func__));
    if (!document) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed").d("reason", "nullDocument"));
        return false;
    }

    ParseResult result = document->Parse(payload);
    if (!result) {
        ACSDK_ERROR(LX("parseDirectivePayloadFailed")
                        .d("reason", "parseFailed")
                        .d("error", GetParseError_En(result.Code()))
                        .d("offset", result.Offset()));
        return false;
    }

    return true;
}

void MessagingCapabilityAgent::handleDirective(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (!info) {
        ACSDK_ERROR(LX("handleDirectiveFailed").d("reason", "nullDirectiveInfo"));
        return;
    }

    m_executor.submit([this, info] {
        const std::string directiveName = info->directive->getName();

        Document payload(kObjectType);
        if (!parseDirectivePayload(info->directive->getPayload(), &payload)) {
            sendExceptionEncounteredAndReportFailed(
                info, "Unable to parse payload", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        if (directiveName == SEND_MESSAGE.name) {
            if (!executeSendMessageDirective(info, payload)) {
                return;
            }
        } else if (directiveName == UPDATE_MESSAGES_STATUS.name) {
            if (!executeUpdateMessagesStatusDirective(info, payload)) {
                return;
            }
        } else if (directiveName == UPLOAD_CONVERSATIONS.name) {
            if (!executeUploadConversationsDirective(info, payload)) {
                return;
            }
        } else {
            sendExceptionEncounteredAndReportFailed(
                info, "Unexpected Directive", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
            return;
        }

        executeSetHandlingCompleted(info);
    });
}

void MessagingCapabilityAgent::cancelDirective(std::shared_ptr<DirectiveInfo> info) {
    removeDirective(info);
}

void MessagingCapabilityAgent::addObserver(std::shared_ptr<MessagingObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("addObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.insert(observer);
}

void MessagingCapabilityAgent::removeObserver(std::shared_ptr<MessagingObserverInterface> observer) {
    if (!observer) {
        ACSDK_ERROR(LX("removeObserverFailed").d("reason", "nullObserver"));
        return;
    }

    std::lock_guard<std::mutex> lock{m_observerMutex};
    m_observers.erase(observer);
}

void MessagingCapabilityAgent::removeDirective(std::shared_ptr<DirectiveInfo> info) {
    if (info->directive && info->result) {
        CapabilityAgent::removeDirective(info->directive->getMessageId());
    }
}

void MessagingCapabilityAgent::executeSetHandlingCompleted(std::shared_ptr<DirectiveInfo> info) {
    ACSDK_DEBUG5(LX(__func__));
    if (info && info->directive && info->result) {
        info->result->setCompleted();
    }
    removeDirective(info);
}

bool MessagingCapabilityAgent::executeSendMessageDirective(
    std::shared_ptr<DirectiveInfo> info,
    rapidjson::Document& payload) {
    rapidjson::Value::ConstMemberIterator it, it2;
    rapidjson::Value endpointInfo;
    std::string token, name;

    // Validate token field
    if (!findNode(payload, JSON_KEY_TOKEN, &it) || !retrieveValue(payload, JSON_KEY_TOKEN, &token) || token.empty()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'token' is not found or empty.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    // Validate messaging endpoint { "messagingEndpointInfo" : { "name" : "DEFAULT"} }
    if (!findNode(payload, JSON_KEY_MESSAGING_ENDPOINT_INFO, &it) || !it->value.IsObject() ||
        !findNode(it->value, JSON_KEY_MESSAGING_ENDPOINT_NAME, &it2) ||
        !retrieveValue(it->value, JSON_KEY_MESSAGING_ENDPOINT_NAME, &name)) {
        sendExceptionEncounteredAndReportFailed(
            info, "'messagingEndpointInfo' is not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    MessagingEndpoint endpoint;
    // Validate against known endpoint values
    if (name == "DEFAULT") {
        endpoint = MessagingEndpoint::DEFAULT;
    } else {
        sendExceptionEncounteredAndReportFailed(
            info, "'name' value is invalid.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        if (observer) {
            observer->sendMessage(token, endpoint, info->directive->getPayload());
        }
    }

    return true;
}

bool MessagingCapabilityAgent::executeUpdateMessagesStatusDirective(
    std::shared_ptr<DirectiveInfo> info,
    rapidjson::Document& payload) {
    rapidjson::Value::ConstMemberIterator it, it2;
    rapidjson::Value endpointInfo;
    std::string token, name, conversationId;

    // Validate token field
    if (!findNode(payload, JSON_KEY_TOKEN, &it) || !retrieveValue(payload, JSON_KEY_TOKEN, &token) || token.empty()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'token' is not found or empty.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    // Validate messaging endpoint { "messagingEndpointInfo" : { "name" : "DEFAULT"} }
    if (!findNode(payload, JSON_KEY_MESSAGING_ENDPOINT_INFO, &it) || !it->value.IsObject() ||
        !findNode(it->value, JSON_KEY_MESSAGING_ENDPOINT_NAME, &it2) ||
        !retrieveValue(it->value, JSON_KEY_MESSAGING_ENDPOINT_NAME, &name)) {
        sendExceptionEncounteredAndReportFailed(
            info, "'messagingEndpointInfo' is not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    MessagingEndpoint endpoint;
    // Validate against known endpoint values
    if (name == "DEFAULT") {
        endpoint = MessagingEndpoint::DEFAULT;
    } else {
        sendExceptionEncounteredAndReportFailed(
            info, "'name' value is invalid.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    // Validate conversationId field
    if (!findNode(payload, JSON_KEY_CONVERSATION_ID, &it) ||
        !retrieveValue(payload, JSON_KEY_CONVERSATION_ID, &conversationId) || conversationId.empty()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'conversationId' is not found or empty.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    // Validate statusMap field
    if (!findNode(payload, JSON_KEY_STATUS_MAP, &it) || !it->value.IsObject()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'statusMap' is not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        if (observer) {
            observer->updateMessagesStatus(token, endpoint, info->directive->getPayload());
        }
    }

    return true;
}

bool MessagingCapabilityAgent::executeUploadConversationsDirective(
    std::shared_ptr<DirectiveInfo> info,
    rapidjson::Document& payload) {
    rapidjson::Value::ConstMemberIterator it, it2;
    std::string token, name;

    // Validate token field
    if (!findNode(payload, JSON_KEY_TOKEN, &it) || !retrieveValue(payload, JSON_KEY_TOKEN, &token) || token.empty()) {
        sendExceptionEncounteredAndReportFailed(
            info, "'token' is not found or empty.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    // Validate messaging endpoint { "messagingEndpointInfo" : { "name" : "DEFAULT"} }
    if (!findNode(payload, JSON_KEY_MESSAGING_ENDPOINT_INFO, &it) || !it->value.IsObject() ||
        !findNode(it->value, JSON_KEY_MESSAGING_ENDPOINT_NAME, &it2) ||
        !retrieveValue(it->value, JSON_KEY_MESSAGING_ENDPOINT_NAME, &name)) {
        sendExceptionEncounteredAndReportFailed(
            info, "'messagingEndpointInfo' is not found.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    MessagingEndpoint endpoint;
    // Validate against known endpoint values
    if (name == "DEFAULT") {
        endpoint = MessagingEndpoint::DEFAULT;
    } else {
        sendExceptionEncounteredAndReportFailed(
            info, "'name' value is invalid.", ExceptionErrorType::UNEXPECTED_INFORMATION_RECEIVED);
        return false;
    }

    std::unique_lock<std::mutex> lock{m_observerMutex};
    auto observers = m_observers;
    lock.unlock();

    for (auto observer : observers) {
        if (observer) {
            observer->uploadConversations(token, endpoint, info->directive->getPayload());
        }
    }

    return true;
}

std::unordered_set<std::shared_ptr<CapabilityConfiguration>> MessagingCapabilityAgent::getCapabilityConfigurations() {
    return m_capabilityConfigurations;
}

void MessagingCapabilityAgent::doShutdown() {
    m_executor.shutdown();
    m_messageSender.reset();
    // Remove state provider
    m_contextManager->removeStateProvider(CONTEXT_MANAGER_MESSAGING_STATE);
    m_contextManager.reset();
}

void MessagingCapabilityAgent::sendMessageSucceeded(const std::string& token, MessagingEndpoint messagingEndpoint) {
    m_executor.submit([=]() {
        avsCommon::utils::json::JsonGenerator jsonGenerator;

        // Add event data
        jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
        jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, messagingEndpointToString(messagingEndpoint));
        jsonGenerator.finishObject();
        jsonGenerator.addMember(JSON_KEY_TOKEN, token);

        // Build event
        auto event = buildJsonEventString(SEND_MESSAGE_SUCCEEDED, "", jsonGenerator.toString());
        auto request = std::make_shared<MessageRequest>(event.second);

        // Send event
        ACSDK_DEBUG5(LX(__func__).sensitive("event", jsonGenerator.toString()));
        m_messageSender->sendMessage(request);
    });
}

void MessagingCapabilityAgent::sendMessageFailed(
    const std::string& token,
    StatusErrorCode code,
    const std::string& message,
    MessagingEndpoint messagingEndpoint) {
    m_executor.submit([=]() {
        avsCommon::utils::json::JsonGenerator jsonGenerator;

        // Add event data
        jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
        jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, messagingEndpointToString(messagingEndpoint));
        jsonGenerator.finishObject();
        jsonGenerator.addMember(JSON_KEY_TOKEN, token);
        jsonGenerator.startObject(JSON_KEY_STATUS);
        jsonGenerator.addMember(JSON_KEY_STATUS_CODE, statusErrorCodeToString(code));
        jsonGenerator.addMember(JSON_KEY_STATUS_MESSAGE, message);
        jsonGenerator.finishObject();

        // Build event
        auto event = buildJsonEventString(SEND_MESSAGE_FAILED, "", jsonGenerator.toString());
        auto request = std::make_shared<MessageRequest>(event.second);

        // Send event
        ACSDK_DEBUG5(LX(__func__).sensitive("event", jsonGenerator.toString()));
        m_messageSender->sendMessage(request);
    });
}

void MessagingCapabilityAgent::updateMessagesStatusSucceeded(
    const std::string& token,
    MessagingEndpoint messagingEndpoint) {
    m_executor.submit([=]() {
        avsCommon::utils::json::JsonGenerator jsonGenerator;

        // Add event data
        jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
        jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, messagingEndpointToString(messagingEndpoint));
        jsonGenerator.finishObject();
        jsonGenerator.addMember(JSON_KEY_TOKEN, token);

        // Build event
        auto event = buildJsonEventString(UPDATE_MESSAGES_STATUS_SUCCEEDED, "", jsonGenerator.toString());
        auto request = std::make_shared<MessageRequest>(event.second);

        // Send event
        ACSDK_DEBUG5(LX(__func__).sensitive("event", jsonGenerator.toString()));
        m_messageSender->sendMessage(request);
    });
}

void MessagingCapabilityAgent::updateMessagesStatusFailed(
    const std::string& token,
    StatusErrorCode code,
    const std::string& message,
    MessagingEndpoint messagingEndpoint) {
    m_executor.submit([=]() {
        avsCommon::utils::json::JsonGenerator jsonGenerator;

        // Add event data
        jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
        jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, messagingEndpointToString(messagingEndpoint));
        jsonGenerator.finishObject();
        jsonGenerator.addMember(JSON_KEY_TOKEN, token);
        jsonGenerator.startObject(JSON_KEY_STATUS);
        jsonGenerator.addMember(JSON_KEY_STATUS_CODE, statusErrorCodeToString(code));
        jsonGenerator.addMember(JSON_KEY_STATUS_MESSAGE, message);
        jsonGenerator.finishObject();

        // Build event
        auto event = buildJsonEventString(UPDATE_MESSAGES_STATUS_FAILED, "", jsonGenerator.toString());
        auto request = std::make_shared<MessageRequest>(event.second);

        // Send event
        ACSDK_DEBUG5(LX(__func__).sensitive("event", jsonGenerator.toString()));
        m_messageSender->sendMessage(request);
    });
}

void MessagingCapabilityAgent::conversationsReport(
    const std::string& token,
    const std::string& conversations,
    UploadMode mode,
    MessagingEndpoint messagingEndpoint) {
    m_executor.submit([=]() {
        avsCommon::utils::json::JsonGenerator jsonGenerator;

        // Add event data
        jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
        jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, messagingEndpointToString(messagingEndpoint));
        jsonGenerator.finishObject();
        jsonGenerator.addMember(JSON_KEY_TOKEN, token);
        jsonGenerator.addRawJsonMember(JSON_KEY_CONVERSATIONS, conversations);
        jsonGenerator.addMember(JSON_KEY_UPLOAD_MODE, uploadModeToString(mode));

        // Build event
        auto event = buildJsonEventString(CONVERSATIONS_REPORT, "", jsonGenerator.toString());
        auto request = std::make_shared<MessageRequest>(event.second);

        // Send event
        ACSDK_DEBUG5(LX(__func__).sensitive("event", jsonGenerator.toString()));
        m_messageSender->sendMessage(request);
    });
}

void MessagingCapabilityAgent::updateMessagingEndpointState(
    MessagingEndpointState messagingEndpointState,
    MessagingEndpoint messagingEndpoint) {
    m_executor.submit([=]() {
        // Update map
        m_messagingEndpointsState[messagingEndpointToString(messagingEndpoint)] = messagingEndpointState;
        executeUpdateMessagingEndpointContext();
    });
}

void MessagingCapabilityAgent::executeUpdateMessagingEndpointContext() {
    // Update context
    avsCommon::utils::json::JsonGenerator jsonGenerator;
    jsonGenerator.startArray(JSON_KEY_MESSAGING_ENDPOINT_STATES);

    for (auto entry : m_messagingEndpointsState) {
        jsonGenerator.startArrayElement();
        jsonGenerator.startObject(JSON_KEY_MESSAGING_ENDPOINT_INFO);
        jsonGenerator.addMember(JSON_KEY_MESSAGING_ENDPOINT_NAME, entry.first);
        jsonGenerator.finishObject();
        jsonGenerator.addMember(JSON_KEY_CONNECTION_STATE, connectionStateToString(entry.second.connection));
        jsonGenerator.startObject(JSON_KEY_MESSAGING_PERMISSIONS);
        jsonGenerator.addMember(
            JSON_KEY_MESSAGING_SEND_PERMISSION, permissionStateToString(entry.second.sendPermission));
        jsonGenerator.addMember(
            JSON_KEY_MESSAGING_READ_PERMISSION, permissionStateToString(entry.second.readPermission));
        jsonGenerator.finishObject();
        jsonGenerator.finishArrayElement();
    }

    jsonGenerator.finishArray();

    // Save new context
    m_messagingContext = jsonGenerator.toString();

    ACSDK_DEBUG5(LX(__func__).sensitive("context", m_messagingContext));
    m_contextManager->reportStateChange(
        CONTEXT_MANAGER_MESSAGING_STATE,
        CapabilityState{m_messagingContext},
        AlexaStateChangeCauseType::APP_INTERACTION);
}

void MessagingCapabilityAgent::executeProvideState(
    const CapabilityTag& stateProviderName,
    const ContextRequestToken contextRequestToken) {
    ACSDK_DEBUG5(LX(__func__).sensitive("context", m_messagingContext));
    CapabilityState state{m_messagingContext};
    m_contextManager->provideStateResponse(stateProviderName, CapabilityState{m_messagingContext}, contextRequestToken);
}

}  // namespace messaging
}  // namespace capabilityAgents
}  // namespace alexaClientSDK