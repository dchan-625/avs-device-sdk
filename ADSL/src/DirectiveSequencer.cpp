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

#include <algorithm>
#include <iostream>
#include <sstream>

#include <AVSCommon/AVS/ExceptionErrorType.h>
#include <AVSCommon/Utils/Logger/Logger.h>
#include <AVSCommon/Utils/Metrics.h>
#include <AVSCommon/Utils/Power/PowerMonitor.h>
#include <AVSCommon/Utils/Power/WakeGuard.h>

#include "ADSL/DirectiveSequencer.h"

/// String to identify log entries originating from this file.
static const std::string TAG("DirectiveSequencer");

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace adsl {

using namespace avsCommon;
using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils;

std::shared_ptr<DirectiveSequencerInterface> DirectiveSequencer::createDirectiveSequencerInterface(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<acsdkShutdownManagerInterfaces::ShutdownNotifierInterface> shutdownNotifier,
    std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface> metricRecorder) {
    ACSDK_DEBUG5(LX(__func__));

    if (!exceptionSender || !shutdownNotifier) {
        ACSDK_ERROR(LX("createDirectiveSequencerInterfaceFailed")
                        .d("isExceptionSenderNull", !exceptionSender)
                        .d("isShutdownNotifierNull", !shutdownNotifier));
        return nullptr;
    }

    auto sequencer = std::shared_ptr<DirectiveSequencer>(new DirectiveSequencer(exceptionSender, metricRecorder));
    shutdownNotifier->addObserver(sequencer);

    return sequencer;
}

std::unique_ptr<DirectiveSequencerInterface> DirectiveSequencer::create(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<metrics::MetricRecorderInterface> metricRecorder) {
    if (!exceptionSender) {
        ACSDK_ERROR(LX("createFailed").d("reason", "nullptrExceptionSender"));
        return nullptr;
    }
    return std::unique_ptr<DirectiveSequencerInterface>(new DirectiveSequencer(exceptionSender, metricRecorder));
}

bool DirectiveSequencer::addDirectiveHandler(std::shared_ptr<DirectiveHandlerInterface> handler) {
    return m_directiveRouter.addDirectiveHandler(handler);
}

bool DirectiveSequencer::removeDirectiveHandler(std::shared_ptr<DirectiveHandlerInterface> handler) {
    return m_directiveRouter.removeDirectiveHandler(handler);
}

void DirectiveSequencer::setDialogRequestId(const std::string& dialogRequestId) {
    m_directiveProcessor->setDialogRequestId(dialogRequestId);
}

std::string DirectiveSequencer::getDialogRequestId() {
    return m_directiveProcessor->getDialogRequestId();
}

bool DirectiveSequencer::onDirective(std::shared_ptr<AVSDirective> directive) {
    if (!directive) {
        ACSDK_ERROR(LX("onDirectiveFailed").d("action", "ignored").d("reason", "nullptrDirective"));
        return false;
    }
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_isShuttingDown || !m_isEnabled) {
        ACSDK_WARN(LX("onDirectiveFailed")
                       .d("directive", directive->getHeaderAsString())
                       .d("action", "ignored")
                       .d("reason", m_isShuttingDown ? "isShuttingDown" : "disabled"));
        return false;
    }
    ACSDK_INFO(LX("onDirective").d("directive0", directive->getHeaderAsString()));
    ACSDK_INFO(LX("onDirective").d("directive1", directive->getUnparsedDirective()));
    m_receivingQueue.push_back(directive);
    m_wakeReceivingLoop.notifyOne();
    return true;
}

DirectiveSequencer::DirectiveSequencer(
    std::shared_ptr<avsCommon::sdkInterfaces::ExceptionEncounteredSenderInterface> exceptionSender,
    std::shared_ptr<metrics::MetricRecorderInterface> metricRecorder) :
        DirectiveSequencerInterface{"DirectiveSequencer"},
        m_mutex{},
        m_exceptionSender{exceptionSender},
        m_isShuttingDown{false},
        m_isEnabled{true},
        m_directiveRouter{metricRecorder},
        m_powerResource{power::PowerMonitor::getInstance()->createLocalPowerResource(TAG)} {
    if (m_powerResource) {
        m_powerResource->acquire();
    }

    m_directiveProcessor = std::make_shared<DirectiveProcessor>(&m_directiveRouter);
    m_receivingThread = std::thread(&DirectiveSequencer::receivingLoop, this);
}

void DirectiveSequencer::doShutdown() {
    ACSDK_DEBUG9(LX("doShutdown"));
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_isShuttingDown = true;
        m_wakeReceivingLoop.notifyOne();
    }
    if (m_receivingThread.joinable()) {
        m_receivingThread.join();
    }
    m_directiveProcessor->shutdown();
    m_directiveRouter.shutdown();
    m_exceptionSender.reset();
}

void DirectiveSequencer::disable() {
    ACSDK_DEBUG9(LX("disable"));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isEnabled = false;
    m_directiveProcessor->setDialogRequestId("");
    m_directiveProcessor->disable();
    m_wakeReceivingLoop.notifyOne();
}

void DirectiveSequencer::enable() {
    ACSDK_DEBUG9(LX("disable"));
    std::lock_guard<std::mutex> lock(m_mutex);
    m_isEnabled = true;
    m_directiveProcessor->enable();
    m_wakeReceivingLoop.notifyOne();
}

void DirectiveSequencer::receivingLoop() {
    auto wake = [this]() { return !m_receivingQueue.empty() || m_isShuttingDown; };

    power::PowerMonitor::getInstance()->assignThreadPowerResource(m_powerResource);

    std::unique_lock<std::mutex> lock(m_mutex);
    while (true) {
        m_wakeReceivingLoop.wait(lock, wake);
        if (m_isShuttingDown) {
            break;
        }
        receiveDirectiveLocked(lock);
    }

    power::PowerMonitor::getInstance()->removeThreadPowerResource();
    if (m_powerResource) {
        m_powerResource->release();
    }
}

void DirectiveSequencer::receiveDirectiveLocked(std::unique_lock<std::mutex>& lock) {
    if (m_receivingQueue.empty()) {
        return;
    }
    auto directive = m_receivingQueue.front();
    m_receivingQueue.pop_front();
    lock.unlock();

    if (directive->getName() == "StopCapture" || directive->getName() == "Speak") {
        ACSDK_METRIC_MSG(TAG, directive, Metrics::Location::ADSL_DEQUEUE);
    }

    bool handled;

/**
 * Previously it was expected that all directives resulting from a Recognize event
 * would be tagged with the dialogRequestId of that event.  In practice that is not
 * the observed behavior.
 */
#ifdef DIALOG_REQUEST_ID_IN_ALL_RESPONSE_DIRECTIVES
    if (directive->getDialogRequestId().empty()) {
        handled = m_directiveRouter.handleDirectiveImmediately(directive);
    } else {
        handled = m_directiveProcessor->onDirective(directive);
    }
#else
    handled = m_directiveProcessor->onDirective(directive);
#endif

    if (!handled) {
        ACSDK_INFO(LX("sendingExceptionEncountered").d("messageId", directive->getMessageId()));
        m_exceptionSender->sendExceptionEncountered(
            directive->getUnparsedDirective(), ExceptionErrorType::UNSUPPORTED_OPERATION, "Unsupported operation");
    }
    lock.lock();
}

}  // namespace adsl
}  // namespace alexaClientSDK