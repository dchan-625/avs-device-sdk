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
#ifndef ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ENDPOINTRESOURCES_H_
#define ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ENDPOINTRESOURCES_H_

#include <string>
#include <vector>

#include <AVSCommon/SDKInterfaces/LocaleAssetsManagerInterface.h>
#include <AVSCommon/Utils/Optional.h>

namespace alexaClientSDK {
namespace avsCommon {
namespace avs {

class EndpointResources {
public:
    /**
     * The constructor.
     */
    EndpointResources();

    /**
     * Function to add friendly name using asset id.
     * 
     * @param assetId The asset id of the friendly name.
     * @return This instance to facilitate setting more information to this EndpointResources.
     */
    EndpointResources& addFriendlyNameWithAssetId(const std::string& assetId);

    /**
     * Function to add friendly name using text value and its locale.
     *
     * @note When using this method it is recommended to provide the friendly names
     * in all the Alexa supported languages. See the class-level link to find the currently
     * supported languages.
     *
     * @note Providing an unsupported locale will result in Discovery failure.
     *
     * @param text The text of the friendly name. This value can contain up to 128 valid characters.
     * @param locale The non-empty locale of the friendly name. 
     * @return This instance to facilitate setting more information to this EndpointResources.
     */
    EndpointResources& addFriendlyNameWithText(
            const std::string& text,
            const avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale);

    /**
     * Function to add manufacturer name using asset id.
     *
     * @param assetId The asset id of the manufacturer name using @c string.
     * @return This instance to facilitate setting more information to this EndpointResources.
     */
    EndpointResources& addManufacturerNameWithAssetId(const std::string& assetId);

    /**
     * Function to add manufacturer name using text value and its locale.
     *
     * @note When using this method it is recommended to provide the manufacturer name
     * in all the Alexa supported languages. See the class-level link to find the currently
     * supported languages.
     *
     * @note Providing an unsupported locale will result in Discovery failure.
     *
     * @param text The text of the manufacturer name. This value can contain up to 128 valid characters.
     * @param locale The non-empty locale of the manufacturer name.
     * @return This instance to facilitate setting more information to this EndpointResources.
     */
    EndpointResources& addManufacturerNameWithText(
            const std::string& text,
            const avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale);

    /**
     * Function to add description using asset id.
     *
     * @param assetId The asset id of the description using @c string.
     * @return This instance to facilitate setting more information to this EndpointResources.
     */
    EndpointResources& addDescriptionWithAssetId(const std::string& assetId);

    /**
     * Function to add description using text value and its locale.
     *
     * @note When using this method it is recommended to provide the description
     * in all the Alexa supported languages. See the class-level link to find the currently
     * supported languages.
     *
     * @note Providing an unsupported locale will result in Discovery failure.
     *
     * @param text The text of the description. This value can contain up to 128 valid characters.
     * @param locale The non-empty locale of the description.
     * @return This instance to facilitate setting more information to this EndpointResources.
     */
    EndpointResources& addDescriptionWithText(
            const std::string& text,
            const avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale& locale);

    /**
     * Function to check if the @c EndpointResources is valid.
     *
     * @return Return @c true if valid, otherwise @c false.
     */
     bool isValid() const;

    /**
     * Builds a new EndpointResources with the configured properties.
     *
     * Build will fail if any attribute is invalid or if a mandatory attribute is missing.
     *
     * @return A json string representing the EndpointResources; otherwise, an empty string.
     */
     std::string build() const;

private:
    /**
     * Struct defining a Label, used to describe a resource.
     * @see https://developer.amazon.com/docs/device-apis/resources-and-assets.html#capability-resources
     */
    struct Label {
        /// The enum representing the Label type.
        enum class LabelType {
            /// Asset type.
            ASSET,

            /// Text type.
            TEXT
        };

        /// The type of the Label. 
        LabelType type;

        /// The value to contain the text or the asset id of the friendly name, manufacturer name or description.
        std::string value;

        /// The locale of the text, and empty object for asset.
        avsCommon::utils::Optional<avsCommon::sdkInterfaces::LocaleAssetsManagerInterface::Locale> locale;

        /**
         *  @name Comparison operator.
         *
         *  Compare the current Label against a second object.
         *  Defined for std::find.
         *
         *  @param rhs The object to compare against this.
         *  @return @c true if the comparison holds; @c false otherwise.
         */
        /// @{
        bool operator==(const Label& rhs) const;
        /// @}

        /**
         * Helper function to convert a Label to a json string.
         *
         * @return A json string of Label.
         */
        std::string toJson() const;
    };

    /// Flag to indicate if there was any error noted.
    bool m_isValid = false;

    /// Vector holding @c Label for the friendly names.
    std::vector<Label> m_friendlyNames;

    /// @c Label that holds the manufacturer name.
    Label m_manufacturerName;

    /// @c Label that holds the description.
    Label m_description;
};

}  // namespace avs
}  // namespace avsCommon
}  // namespace alexaClientSDK

#endif //ALEXA_CLIENT_SDK_AVSCOMMON_AVS_INCLUDE_AVSCOMMON_AVS_ENDPOINTRESOURCES_H_
