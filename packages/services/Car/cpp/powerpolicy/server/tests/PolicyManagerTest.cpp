/**
 * Copyright (c) 2020, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "PolicyManager.h"

#include <android-base/file.h>
#include <android/hardware/automotive/vehicle/2.0/IVehicle.h>
#include <gmock/gmock.h>

#include <unordered_set>

namespace android {
namespace frameworks {
namespace automotive {
namespace powerpolicy {

using android::hardware::automotive::vehicle::V2_0::VehicleApPowerStateReport;
using tinyxml2::XML_SUCCESS;
using tinyxml2::XMLDocument;

namespace {

constexpr const char* kDirPrefix = "/tests/data/";

constexpr const char* kValidPowerPolicyXmlFile = "valid_power_policy.xml";
constexpr const char* kValidPowerPolicyNoPowerPolicyGroupsXmlFile =
        "valid_power_policy_no_power_policy_groups.xml";
constexpr const char* kValidPowerPolicyNoSystemPowerPolicyXmlFile =
        "valid_power_policy_no_system_power_policy.xml";
constexpr const char* kValidPowerPolicyPowerPoliciesOnlyXmlFile =
        "valid_power_policy_policies_only.xml";
constexpr const char* kValidPowerPolicySystemPowerPolicyOnlyXmlFile =
        "valid_power_policy_system_power_policy_only.xml";
const std::vector<const char*> kInvalidPowerPolicyXmlFiles =
        {"invalid_power_policy_incorrect_id.xml",
         "invalid_power_policy_incorrect_othercomponent.xml",
         "invalid_power_policy_incorrect_value.xml", "invalid_power_policy_unknown_component.xml"};
const std::vector<const char*> kInvalidPowerPolicyGroupXmlFiles =
        {"invalid_power_policy_group_incorrect_state.xml",
         "invalid_power_policy_group_missing_policy.xml"};
const std::vector<const char*> kInvalidSystemPowerPolicyXmlFiles =
        {"invalid_system_power_policy_incorrect_component.xml",
         "invalid_system_power_policy_incorrect_id.xml"};

constexpr const char* kExistingPowerPolicyId = "expected_to_be_registered";
constexpr const char* kExistingPowerPolicyId_OtherOff = "policy_id_other_off";
constexpr const char* kExistingPowerPolicyId_OtherOn = "policy_id_other_on";
constexpr const char* kExistingPowerPolicyId_OtherUntouched = "policy_id_other_untouched";
constexpr const char* kExistingPowerPolicyId_OtherNone = "policy_id_other_none";
constexpr const char* kNonExistingPowerPolicyId = "non_existing_power_poicy_id";
constexpr const char* kValidPowerPolicyGroupId = "mixed_policy_group";
constexpr const char* kInvalidPowerPolicyGroupId = "invalid_policy_group";
constexpr const char* kSystemPolicyIdNoUserInteraction = "system_power_policy_no_user_interaction";

const VehicleApPowerStateReport kExistingTransition = VehicleApPowerStateReport::WAIT_FOR_VHAL;
const VehicleApPowerStateReport kNonExistingTransition = static_cast<VehicleApPowerStateReport>(-1);

CarPowerPolicy createCarPowerPolicy(const std::string& id,
                                    const std::vector<PowerComponent>& enabledComponents,
                                    const std::vector<PowerComponent>& disabledComponents) {
    CarPowerPolicy policy;
    policy.policyId = id;
    policy.enabledComponents = enabledComponents;
    policy.disabledComponents = disabledComponents;
    return policy;
}

const CarPowerPolicy kExistingPowerPolicy_OtherOff =
        createCarPowerPolicy("policy_id_other_off", {PowerComponent::WIFI},
                             {PowerComponent::AUDIO, PowerComponent::MEDIA, PowerComponent::DISPLAY,
                              PowerComponent::BLUETOOTH, PowerComponent::CELLULAR,
                              PowerComponent::ETHERNET, PowerComponent::PROJECTION,
                              PowerComponent::NFC, PowerComponent::INPUT,
                              PowerComponent::VOICE_INTERACTION, PowerComponent::VISUAL_INTERACTION,
                              PowerComponent::TRUSTED_DEVICE_DETECTION, PowerComponent::LOCATION,
                              PowerComponent::MICROPHONE, PowerComponent::CPU});
const CarPowerPolicy kExistingPowerPolicy_OtherOn =
        createCarPowerPolicy("policy_id_other_on",
                             {PowerComponent::MEDIA, PowerComponent::DISPLAY,
                              PowerComponent::BLUETOOTH, PowerComponent::WIFI,
                              PowerComponent::CELLULAR, PowerComponent::ETHERNET,
                              PowerComponent::PROJECTION, PowerComponent::NFC,
                              PowerComponent::INPUT, PowerComponent::LOCATION,
                              PowerComponent::MICROPHONE, PowerComponent::CPU},
                             {PowerComponent::AUDIO, PowerComponent::VOICE_INTERACTION,
                              PowerComponent::VISUAL_INTERACTION,
                              PowerComponent::TRUSTED_DEVICE_DETECTION});
const CarPowerPolicy kExistingPowerPolicy_OtherUntouched =
        createCarPowerPolicy("policy_id_other_untouched",
                             {PowerComponent::AUDIO, PowerComponent::DISPLAY,
                              PowerComponent::BLUETOOTH, PowerComponent::WIFI,
                              PowerComponent::VOICE_INTERACTION, PowerComponent::VISUAL_INTERACTION,
                              PowerComponent::TRUSTED_DEVICE_DETECTION},
                             {});
const CarPowerPolicy kExistingPowerPolicy_OtherNone =
        createCarPowerPolicy("policy_id_other_none", {PowerComponent::WIFI},
                             {PowerComponent::AUDIO, PowerComponent::VOICE_INTERACTION,
                              PowerComponent::VISUAL_INTERACTION,
                              PowerComponent::TRUSTED_DEVICE_DETECTION});
const CarPowerPolicy& kExistingTransitionPolicy = kExistingPowerPolicy_OtherOn;
const CarPowerPolicy kSystemPowerPolicyNoUserInteraction =
        createCarPowerPolicy("system_power_policy_no_user_interaction",
                             {PowerComponent::WIFI, PowerComponent::CELLULAR,
                              PowerComponent::ETHERNET, PowerComponent::TRUSTED_DEVICE_DETECTION,
                              PowerComponent::CPU},
                             {PowerComponent::AUDIO, PowerComponent::MEDIA, PowerComponent::DISPLAY,
                              PowerComponent::BLUETOOTH, PowerComponent::PROJECTION,
                              PowerComponent::NFC, PowerComponent::INPUT,
                              PowerComponent::VOICE_INTERACTION, PowerComponent::VISUAL_INTERACTION,
                              PowerComponent::LOCATION, PowerComponent::MICROPHONE});
const CarPowerPolicy kModifiedSystemPowerPolicy =
        createCarPowerPolicy("system_power_policy_no_user_interaction",
                             {PowerComponent::BLUETOOTH, PowerComponent::WIFI,
                              PowerComponent::CELLULAR, PowerComponent::ETHERNET,
                              PowerComponent::NFC, PowerComponent::CPU},
                             {PowerComponent::AUDIO, PowerComponent::MEDIA, PowerComponent::DISPLAY,
                              PowerComponent::PROJECTION, PowerComponent::INPUT,
                              PowerComponent::VOICE_INTERACTION, PowerComponent::VISUAL_INTERACTION,
                              PowerComponent::TRUSTED_DEVICE_DETECTION, PowerComponent::LOCATION,
                              PowerComponent::MICROPHONE});

std::string getTestDataPath(const char* filename) {
    static std::string baseDir = android::base::GetExecutableDirectory();
    return baseDir + kDirPrefix + filename;
}

bool compareComponents(const std::vector<PowerComponent>& a, const std::vector<PowerComponent>& b) {
    int lenA = a.size();
    int lenB = b.size();
    if (lenA != lenB) {
        return false;
    }
    std::unordered_set<PowerComponent> componentSet;
    for (const auto component : a) {
        componentSet.insert(component);
    }
    for (const auto component : b) {
        if (componentSet.count(component) == 0) {
            return false;
        }
        componentSet.erase(component);
    }
    return componentSet.size() == 0;
}

bool isEqual(const CarPowerPolicy& a, const CarPowerPolicy& b) {
    return a.policyId == a.policyId &&
            compareComponents(a.enabledComponents, b.enabledComponents) &&
            compareComponents(a.disabledComponents, b.disabledComponents);
}

void checkPolicies(const PolicyManager& policyManager) {
    ASSERT_FALSE(policyManager.getPowerPolicy(kNonExistingPowerPolicyId).ok());

    // otherComponents behavior = off
    auto policyMeta = policyManager.getPowerPolicy(kExistingPowerPolicyId_OtherOff);
    ASSERT_TRUE(policyMeta.ok());
    ASSERT_TRUE(isEqual(*policyMeta->powerPolicy, kExistingPowerPolicy_OtherOff));
    // otherComponents behavior = on
    policyMeta = policyManager.getPowerPolicy(kExistingPowerPolicyId_OtherOn);
    ASSERT_TRUE(policyMeta.ok());
    ASSERT_TRUE(isEqual(*policyMeta->powerPolicy, kExistingPowerPolicy_OtherOn));
    // otherComponents behavior = untouched
    policyMeta = policyManager.getPowerPolicy(kExistingPowerPolicyId_OtherUntouched);
    ASSERT_TRUE(policyMeta.ok());
    ASSERT_TRUE(isEqual(*policyMeta->powerPolicy, kExistingPowerPolicy_OtherUntouched));
    // otherComponents behavior = none
    policyMeta = policyManager.getPowerPolicy(kExistingPowerPolicyId_OtherNone);
    ASSERT_TRUE(policyMeta.ok());
    ASSERT_TRUE(isEqual(*policyMeta->powerPolicy, kExistingPowerPolicy_OtherNone));
}

void checkPowerPolicyGroups(const PolicyManager& policyManager) {
    auto policy = policyManager.getDefaultPowerPolicyForState(kValidPowerPolicyGroupId,
                                                              kExistingTransition);
    ASSERT_TRUE(policy.ok());
    ASSERT_TRUE(isEqual(**policy, kExistingTransitionPolicy));
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kNonExistingTransition)
                    .ok());
}

void checkSystemPowerPolicy(const PolicyManager& policyManager,
                            const CarPowerPolicy& expectedPolicy) {
    auto policyMeta = policyManager.getPowerPolicy(kSystemPolicyIdNoUserInteraction);
    ASSERT_TRUE(isEqual(*policyMeta->powerPolicy, expectedPolicy));
}

void checkInvalidPolicies(const PolicyManager& policyManager) {
    ASSERT_FALSE(policyManager.getPowerPolicy(kExistingPowerPolicyId).ok());
    ASSERT_FALSE(policyManager.getPowerPolicy(kNonExistingPowerPolicyId).ok());
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kExistingTransition)
                    .ok());
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kNonExistingTransition)
                    .ok());
    auto policyMeta = policyManager.getPowerPolicy(kSystemPolicyIdNoUserInteraction);
    ASSERT_TRUE(isEqual(*policyMeta->powerPolicy, kSystemPowerPolicyNoUserInteraction));
}

}  // namespace

namespace internal {

class PolicyManagerPeer {
public:
    explicit PolicyManagerPeer(PolicyManager* manager) : mManager(manager) {
        manager->initRegularPowerPolicy();
        manager->initPreemptivePowerPolicy();
    }

    void expectValidPowerPolicyXML(const char* filename) { readXmlFile(filename); }
    void expectInvalidPowerPolicyXML(const char* filename) { readXmlFile(filename); }

private:
    void readXmlFile(const char* filename) {
        XMLDocument xmlDoc;
        std::string path = getTestDataPath(filename);
        xmlDoc.LoadFile(path.c_str());
        ASSERT_TRUE(xmlDoc.ErrorID() == XML_SUCCESS);
        mManager->readPowerPolicyFromXml(xmlDoc);
    }

private:
    PolicyManager* mManager;
};

}  // namespace internal

class PolicyManagerTest : public ::testing::Test {};

TEST_F(PolicyManagerTest, TestValidXml_PowerPolicy) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyXmlFile);

    checkPolicies(policyManager);
}

TEST_F(PolicyManagerTest, TestValidXml_PowerPolicyGroup) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyXmlFile);

    checkPowerPolicyGroups(policyManager);
}

TEST_F(PolicyManagerTest, TestValidXml_SystemPowerPolicy) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyXmlFile);

    checkSystemPowerPolicy(policyManager, kModifiedSystemPowerPolicy);
}

TEST_F(PolicyManagerTest, TestValidXml_NoPowerPolicyGroups) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyNoPowerPolicyGroupsXmlFile);

    checkPolicies(policyManager);
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kExistingTransition)
                    .ok());
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kNonExistingTransition)
                    .ok());
    checkSystemPowerPolicy(policyManager, kModifiedSystemPowerPolicy);
}

TEST_F(PolicyManagerTest, TestValidXml_NoSystemPowerPolicy) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyNoSystemPowerPolicyXmlFile);

    checkPolicies(policyManager);
    checkPowerPolicyGroups(policyManager);
    checkSystemPowerPolicy(policyManager, kSystemPowerPolicyNoUserInteraction);
}

TEST_F(PolicyManagerTest, TestValidXml_PoliciesOnly) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyPowerPoliciesOnlyXmlFile);

    checkPolicies(policyManager);
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kExistingTransition)
                    .ok());
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kNonExistingTransition)
                    .ok());
    checkSystemPowerPolicy(policyManager, kSystemPowerPolicyNoUserInteraction);
}

TEST_F(PolicyManagerTest, TestValidXml_SystemPowerPolicyOnly) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicySystemPowerPolicyOnlyXmlFile);

    ASSERT_FALSE(policyManager.getPowerPolicy(kExistingPowerPolicyId).ok());
    ASSERT_FALSE(policyManager.getPowerPolicy(kNonExistingPowerPolicyId).ok());
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kExistingTransition)
                    .ok());
    ASSERT_FALSE(
            policyManager
                    .getDefaultPowerPolicyForState(kValidPowerPolicyGroupId, kNonExistingTransition)
                    .ok());
    checkSystemPowerPolicy(policyManager, kModifiedSystemPowerPolicy);
}

TEST_F(PolicyManagerTest, TestInvalidPowerPolicyXml) {
    for (const auto& filename : kInvalidPowerPolicyXmlFiles) {
        PolicyManager policyManager;
        internal::PolicyManagerPeer policyManagerPeer(&policyManager);
        policyManagerPeer.expectInvalidPowerPolicyXML(filename);

        checkInvalidPolicies(policyManager);
    }
}

TEST_F(PolicyManagerTest, TestInvalidPowerPolicyGroupXml) {
    for (const auto& filename : kInvalidPowerPolicyGroupXmlFiles) {
        PolicyManager policyManager;
        internal::PolicyManagerPeer policyManagerPeer(&policyManager);
        policyManagerPeer.expectInvalidPowerPolicyXML(filename);

        checkInvalidPolicies(policyManager);
    }
}

TEST_F(PolicyManagerTest, TestInvalidSystemPowerPolicyXml) {
    for (const auto& filename : kInvalidSystemPowerPolicyXmlFiles) {
        PolicyManager policyManager;
        internal::PolicyManagerPeer policyManagerPeer(&policyManager);
        policyManagerPeer.expectInvalidPowerPolicyXML(filename);

        checkInvalidPolicies(policyManager);
    }
}

TEST_F(PolicyManagerTest, TestValidXml_PowerPolicyGroupAvailable) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    policyManagerPeer.expectValidPowerPolicyXML(kValidPowerPolicyXmlFile);

    ASSERT_TRUE(policyManager.isPowerPolicyGroupAvailable(kValidPowerPolicyGroupId));
    ASSERT_FALSE(policyManager.isPowerPolicyGroupAvailable(kInvalidPowerPolicyGroupId));
}

TEST_F(PolicyManagerTest, TestSystemPowerPolicyAllOn) {
    PolicyManager policyManager;
    internal::PolicyManagerPeer policyManagerPeer(&policyManager);
    std::unordered_set<PowerComponent> enabledComponentSet;
    auto policyMeta = policyManager.getPowerPolicy("system_power_policy_all_on");

    ASSERT_TRUE(policyMeta.ok());

    CarPowerPolicyPtr systemPolicyDefault = policyMeta->powerPolicy;
    for (const auto& component : systemPolicyDefault->enabledComponents) {
        enabledComponentSet.insert(component);
    }
    for (const auto component : enum_range<PowerComponent>()) {
        ASSERT_GT(enabledComponentSet.count(component), 0);
        enabledComponentSet.erase(component);
    }

    ASSERT_TRUE(enabledComponentSet.empty());
    ASSERT_TRUE(systemPolicyDefault->disabledComponents.empty());
}

}  // namespace powerpolicy
}  // namespace automotive
}  // namespace frameworks
}  // namespace android
