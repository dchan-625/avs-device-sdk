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

#include "RegistrationManager/RegistrationManager.h"

#include "AVSCommon/Utils/Logger/Logger.h"
#include "RegistrationManager/CustomerDataManager.h"
#include <AVSCommon/Utils/Metrics/DataPointCounterBuilder.h>
#include <AVSCommon/Utils/Metrics/MetricEventBuilder.h>

/// String to identify log entries originating from this file.
static const std::string TAG("RegistrationManager");

/// The metric source prefix string.
static const std::string METRIC_SOURCE_PREFIX = "REGISTRATION_MANAGER-";

/// The logout occurred metric string.
static const std::string LOGOUT_OCCURRED = "LOGOUT_OCCURRED";

/**
 * Create a LogEntry using this file's TAG and the specified event string.
 *
 * @param The event string for this @c LogEntry.
 */
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

namespace alexaClientSDK {
namespace registrationManager {

using namespace avsCommon::utils::metrics;

/**
 * Submits a logout occurred metric to the metric recorder.
 * @param metricRecorder - The @c MetricRecorderInterface pointer.
 */
static void submitLogoutMetric(const std::shared_ptr<MetricRecorderInterface>& metricRecorder) {
    if (!metricRecorder) {
        return;
    }

    auto metricEvent = MetricEventBuilder{}
                           .setActivityName(METRIC_SOURCE_PREFIX + LOGOUT_OCCURRED)
                           .addDataPoint(DataPointCounterBuilder{}.setName(LOGOUT_OCCURRED).increment(1).build())
                           .build();

    if (!metricEvent) {
        ACSDK_ERROR(LX("submitLogoutMetricFailed").d("reason", "null metric event"));
        return;
    }

    recordMetric(metricRecorder, metricEvent);
}

void RegistrationManager::logout() {
    ACSDK_DEBUG(LX("logout"));
    m_directiveSequencer->disable();
    m_connectionManager->disable();
    m_dataManager->clearData();
    notifyObservers();
    submitLogoutMetric(m_metricRecorder);
}

RegistrationManager::RegistrationManager(
    std::shared_ptr<avsCommon::sdkInterfaces::DirectiveSequencerInterface> directiveSequencer,
    std::shared_ptr<avsCommon::sdkInterfaces::AVSConnectionManagerInterface> connectionManager,
    std::shared_ptr<CustomerDataManager> dataManager,
    const std::shared_ptr<avsCommon::utils::metrics::MetricRecorderInterface>& metricRecorder) :
        m_directiveSequencer{directiveSequencer},
        m_connectionManager{connectionManager},
        m_dataManager{dataManager},
        m_metricRecorder{metricRecorder} {
    if (!directiveSequencer) {
        ACSDK_ERROR(LX("RegistrationManagerFailed").m("Invalid directiveSequencer."));
    }
    if (!connectionManager) {
        ACSDK_ERROR(LX("RegistrationManagerFailed").m("Invalid connectionManager."));
    }
    if (!dataManager) {
        ACSDK_ERROR(LX("RegistrationManagerFailed").m("Invalid dataManager."));
    }
}

void RegistrationManager::addObserver(std::shared_ptr<RegistrationObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.insert(observer);
}

void RegistrationManager::removeObserver(std::shared_ptr<RegistrationObserverInterface> observer) {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    m_observers.erase(observer);
}

void RegistrationManager::notifyObservers() {
    std::lock_guard<std::mutex> lock{m_observersMutex};
    for (auto& observer : m_observers) {
        observer->onLogout();
    }
}

}  // namespace registrationManager
}  // namespace alexaClientSDK
