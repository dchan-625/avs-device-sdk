/*
 * Copyright 2020 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <AVSCommon/AVS/EndpointResources.h>
#include <AVSCommon/Utils/JSON/JSONGenerator.h>
#include <AVSCommon/Utils/Logger/Logger.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

using namespace avsCommon::utils;

/// String to identify log entries originating from this file.
static const std::string TAG("EndpointResources");

/**
* Create a LogEntry using this file's TAG and the specified event string.
*
* @param The event string for this @c LogEntry.
*/
#define LX(event) alexaClientSDK::avsCommon::utils::logger::LogEntry(TAG, event)

/// Maximum length of the friendly name
static constexpr size_t MAX_FRIENDLY_NAME_LENGTH = 128;
/// Maximum length of the manufacturer name
static constexpr size_t MAX_MANUFACTURER_NAME_LENGTH = 128;
/// Maximum length of the description
static constexpr size_t MAX_DESCRIPTION_LENGTH = 128;

EndpointResources::EndpointResources() : m_isValid{true} {
}

bool EndpointResources::Label::operator==(const EndpointResources::Label& rhs) const {
    return value == rhs.value && locale.valueOr("") == rhs.locale.valueOr("");
}

EndpointResources& EndpointResources::addFriendlyNameWithAssetId(const std::string& assetId) {
    if (assetId.length() == 0) {
        ACSDK_ERROR(LX("addFriendlyNameWithAssetIdFailed").d("reason", "invalidAssetId"));
        m_isValid = false;
        return *this;
    }

    if (std::find(
            m_friendlyNames.begin(),
            m_friendlyNames.end(),
            EndpointResources::Label(
                    {Label::LabelType::ASSET, assetId, utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale>()})) != m_friendlyNames.end()) {
        ACSDK_ERROR(LX("addFriendlyNameWithAssetIdFailed").d("reason", "duplicateAssetId").sensitive("assetId", assetId));
        m_isValid = false;
        return *this;
    }

    m_friendlyNames.push_back({Label::LabelType::ASSET, assetId, utils::Optional<sdkInterfaces::LocaleAssetsManagerInterface::Locale>()});

    return *this;
};

EndpointResources& EndpointResources::addFriendlyNameWithText(
        const std::string& text,
        const avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale) {

        if (text.length() == 0 || text.length() > MAX_FRIENDLY_NAME_LENGTH) {
            ACSDK_ERROR(LX("addFriendlyNameWithTextFailed").d("reason", "invalidText"));
            m_isValid = false;
            return *this;
        }
        if (locale.empty()) {
            ACSDK_ERROR(LX("addFriendlyNameWithTextFailed").d("reason", "invalidLocale"));
            m_isValid = false;
            return *this;
        }
        if (std::find(
                m_friendlyNames.begin(),
                m_friendlyNames.end(),
                Label({Label::LabelType::TEXT, text, Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale>(locale)})) !=
                m_friendlyNames.end()) {
            ACSDK_WARN(LX("addFriendlyNameWithTextFailed")
                                .d("reason", "duplicateText")
                                .sensitive("text", text)
                                .sensitive("locale", locale));
            return *this;
        }

        m_friendlyNames.push_back({Label::LabelType::TEXT, text, Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale>(locale)});

    return *this;
};

EndpointResources& EndpointResources::addManufacturerNameWithAssetId(const std::string& assetId) {
    if (assetId.length() == 0){
        ACSDK_ERROR(LX("addManufacturerNameWithAssetIdFailed").d("reason", "invalidAssetId"));
        m_isValid = false;
        return *this;
    }
    if (m_manufacturerName.value.length() != 0){
        ACSDK_ERROR(LX("addManufacturerNameWithAssetIdFailed").d("reason", "manufacturerNameAlreadyExists"));
        m_isValid = false;
        return *this;
    }

    m_manufacturerName = {Label::LabelType::ASSET, assetId, Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale>()};

    return *this;
};

EndpointResources& EndpointResources::addManufacturerNameWithText(
        const std::string& text,
        const avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale) {

    if (text.length() == 0 || text.length() > MAX_MANUFACTURER_NAME_LENGTH) {
        ACSDK_ERROR(LX("addManufacturerNameWithTextFailed").d("reason", "invalidText"));
        m_isValid = false;
    }
    if (locale.empty()) {
        ACSDK_ERROR(LX("addManufacturerNameWithTextFailed").d("reason", "invalidLocale"));
        m_isValid = false;
    }
    if (m_manufacturerName.value.length() != 0){
        ACSDK_ERROR(LX("addManufacturerNameWithAssetIdFailed").d("reason", "manufacturerNameAlreadyExists"));
        m_isValid = false;
        return *this;
    }

    m_manufacturerName = {Label::LabelType::TEXT, text, Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale>(locale)};
    return *this;
};


EndpointResources& EndpointResources::addDescriptionWithAssetId(const std::string& assetId) {
    if (assetId.length() == 0){
        ACSDK_ERROR(LX("addDescriptionWithAssetIdFailed").d("reason", "invalidAssetId"));
        m_isValid = false;
        return *this;
    }
    if (m_description.value.length() != 0){
        ACSDK_ERROR(LX("addDescriptionWithAssetIdFailed").d("reason", "descriptionAlreadyExists"));
        m_isValid = false;
        return *this;
    }

    m_description = {Label::LabelType::ASSET, assetId, Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale>()};
    return *this;
};

EndpointResources& EndpointResources::addDescriptionWithText(
        const std::string& text,
        const avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale) {

    if (text.length() == 0 || text.length() > MAX_DESCRIPTION_LENGTH) {
        ACSDK_ERROR(LX("addDescriptionWithTextFailed").d("reason", "invalidText"));
        m_isValid = false;
    }
    if (locale.empty()) {
        ACSDK_ERROR(LX("addDescriptionWithTextFailed").d("reason", "invalidLocale"));
        m_isValid = false;
    }
    if (m_description.value.length() != 0){
        ACSDK_ERROR(LX("addDescriptionWithAssetIdFailed").d("reason", "descriptionAlreadyExists"));
        m_isValid = false;
        return *this;
    }

    m_description = {Label::LabelType::TEXT, text, Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale>(locale)};
    return *this;
};

bool EndpointResources::isValid() const {
    return m_isValid && m_friendlyNames.size() > 0 && m_description.value.length() > 0
                     && m_manufacturerName.value.length() > 0;
};

std::string EndpointResources::build() const {
    if (!isValid()) {
        ACSDK_ERROR(LX("buildFailed").d("reason", "invalidEndpointResources"));
        return "";
    }
    json::JsonGenerator jsonGenerator;
    std::vector<std::string> friendlyNames;
    for (auto &friendlyName : m_friendlyNames){
        friendlyNames.push_back(friendlyName.toJson());
    }
    jsonGenerator.addMembersArray("friendlyNames", friendlyNames);
    jsonGenerator.addRawJsonMember("manufacturerName", m_manufacturerName.toJson());
    jsonGenerator.addRawJsonMember("description", m_description.toJson());
    return jsonGenerator.toString();
};

std::string EndpointResources::Label::toJson() const {
    json::JsonGenerator scopeGenerator;
    if (type == Label::LabelType::TEXT) {
        scopeGenerator.addMember("@type", "text");
        scopeGenerator.startObject("value");
        scopeGenerator.addMember("text", value);
        scopeGenerator.addMember("locale", locale.value());
    } else if (type == Label::LabelType::ASSET) {
        scopeGenerator.addMember("@type", "asset");
        scopeGenerator.startObject("value");
        scopeGenerator.addMember("assetId", value);
    } else{
        return "{}";
    }
    return scopeGenerator.toString();
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK
