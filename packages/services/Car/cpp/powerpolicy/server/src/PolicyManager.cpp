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

#define LOG_TAG "carpowerpolicyd"

#include "PolicyManager.h"

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <binder/Enums.h>

#include <tinyxml2.h>

#include <cstring>
#include <unordered_set>
#include <vector>

namespace android {
namespace frameworks {
namespace automotive {
namespace powerpolicy {

using android::base::Error;
using android::base::Result;
using android::base::StartsWith;
using android::base::StringAppendF;
using android::base::StringPrintf;
using android::base::WriteStringToFd;
using android::hardware::automotive::vehicle::V2_0::VehicleApPowerStateReport;
using tinyxml2::XML_SUCCESS;
using tinyxml2::XMLDocument;
using tinyxml2::XMLElement;

namespace {

// Vendor power policy filename.
constexpr const char* kVendorPolicyFile = "/vendor/etc/automotive/power_policy.xml";

// Tags and attributes in vendor power policy XML file.
constexpr const char* kTagRoot = "powerPolicy";
constexpr const char* kTagPolicyGroups = "policyGroups";
constexpr const char* kTagPolicyGroup = "policyGroup";
constexpr const char* kTagDefaultPolicy = "defaultPolicy";
constexpr const char* kTagNoDefaultPolicy = "noDefaultPolicy";
constexpr const char* kTagPolicies = "policies";
constexpr const char* kTagPolicy = "policy";
constexpr const char* kTagOtherComponents = "otherComponents";
constexpr const char* kTagComponent = "component";
constexpr const char* kTagSystemPolicyOverrides = "systemPolicyOverrides";
constexpr const char* kAttrBehavior = "behavior";
constexpr const char* kAttrId = "id";
constexpr const char* kAttrState = "state";

// Power states.
constexpr const char* kPowerStateOn = "on";
constexpr const char* kPowerStateOff = "off";
constexpr const char* kPowerStateUntouched = "untouched";

// Power transitions that a power policy can be applied with.
constexpr const char* kPowerTransitionWaitForVhal = "WaitForVHAL";
constexpr const char* kPowerTransitionOn = "On";
constexpr const char* kPowerTransitionShutdownStart = "ShutdownStart";
constexpr const char* kPowerTransitionDeepSleepEntry = "DeepSleepEntry";

const PowerComponent INVALID_POWER_COMPONENT = static_cast<PowerComponent>(-1);
const int32_t INVALID_VEHICLE_POWER_STATE = -1;

constexpr const char* kPowerComponentPrefix = "POWER_COMPONENT_";
constexpr const char* kSystemPolicyPrefix = "system_power_policy_";

// System power policy definition: ID, enabled components, and disabled components.
const std::vector<PowerComponent> kNoUserInteractionEnabledComponents =
        {PowerComponent::WIFI, PowerComponent::CELLULAR, PowerComponent::ETHERNET,
         PowerComponent::TRUSTED_DEVICE_DETECTION, PowerComponent::CPU};
const std::vector<PowerComponent> kNoUserInteractionDisabledComponents =
        {PowerComponent::AUDIO,
         PowerComponent::MEDIA,
         PowerComponent::DISPLAY,
         PowerComponent::BLUETOOTH,
         PowerComponent::PROJECTION,
         PowerComponent::NFC,
         PowerComponent::INPUT,
         PowerComponent::VOICE_INTERACTION,
         PowerComponent::VISUAL_INTERACTION,
         PowerComponent::LOCATION,
         PowerComponent::MICROPHONE};
const std::vector<PowerComponent> kAllComponents = {PowerComponent::AUDIO,
                                                    PowerComponent::MEDIA,
                                                    PowerComponent::DISPLAY,
                                                    PowerComponent::BLUETOOTH,
                                                    PowerComponent::WIFI,
                                                    PowerComponent::CELLULAR,
                                                    PowerComponent::ETHERNET,
                                                    PowerComponent::PROJECTION,
                                                    PowerComponent::NFC,
                                                    PowerComponent::INPUT,
                                                    PowerComponent::VOICE_INTERACTION,
                                                    PowerComponent::VISUAL_INTERACTION,
                                                    PowerComponent::TRUSTED_DEVICE_DETECTION,
                                                    PowerComponent::LOCATION,
                                                    PowerComponent::MICROPHONE,
                                                    PowerComponent::CPU};
const std::vector<PowerComponent> kInitialOnComponents = {PowerComponent::AUDIO,
                                                          PowerComponent::DISPLAY,
                                                          PowerComponent::CPU};
const std::vector<PowerComponent> kNoComponents;
const std::vector<PowerComponent> kSuspendToRamDisabledComponents = {PowerComponent::AUDIO,
                                                                     PowerComponent::BLUETOOTH,
                                                                     PowerComponent::WIFI,
                                                                     PowerComponent::LOCATION,
                                                                     PowerComponent::CPU};
const std::unordered_set<PowerComponent> kNoUserInteractionConfigurableComponents =
        {PowerComponent::BLUETOOTH, PowerComponent::NFC, PowerComponent::TRUSTED_DEVICE_DETECTION};

void iterateAllPowerComponents(const std::function<bool(PowerComponent)>& processor) {
    for (const auto component : enum_range<PowerComponent>()) {
        if (!processor(component)) {
            break;
        }
    }
}

PowerComponent toPowerComponent(std::string_view id, std::string_view prefix) {
    if (!StartsWith(id, prefix)) {
        return INVALID_POWER_COMPONENT;
    }
    std::string_view componentId = id.substr(prefix.size());
    PowerComponent matchedComponent = INVALID_POWER_COMPONENT;
    iterateAllPowerComponents([componentId, &matchedComponent](PowerComponent component) -> bool {
        if (componentId == toString(component)) {
            matchedComponent = component;
            return false;
        }
        return true;
    });
    return matchedComponent;
}

const char* safePtrPrint(const char* ptr) {
    return ptr == nullptr ? "nullptr" : ptr;
}

int32_t toVehiclePowerState(const char* state) {
    if (!strcmp(state, kPowerTransitionWaitForVhal)) {
        return static_cast<int32_t>(VehicleApPowerStateReport::WAIT_FOR_VHAL);
    }
    if (!strcmp(state, kPowerTransitionOn)) {
        return static_cast<int32_t>(VehicleApPowerStateReport::ON);
    }
    return INVALID_VEHICLE_POWER_STATE;
}

bool isValidPowerState(int32_t state) {
    return state != INVALID_VEHICLE_POWER_STATE;
}

void logXmlError(const std::string& errMsg) {
    ALOGW("Proceed without registered policies: %s", errMsg.c_str());
}

Result<void> readComponents(const XMLElement* pPolicy, CarPowerPolicyPtr policy,
                            std::unordered_set<PowerComponent>* visited) {
    for (const XMLElement* pComponent = pPolicy->FirstChildElement(kTagComponent);
         pComponent != nullptr; pComponent = pComponent->NextSiblingElement(kTagComponent)) {
        const char* id;
        if (pComponent->QueryStringAttribute(kAttrId, &id) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag", kAttrId,
                                           kTagComponent);
        }
        PowerComponent componentId = toPowerComponent(id, kPowerComponentPrefix);
        if (componentId == INVALID_POWER_COMPONENT) {
            return Error() << StringPrintf("XML configuration has invalid value(%s) in |%s| "
                                           "attribute of |%s| tag",
                                           safePtrPrint(id), kAttrId, kTagComponent);
        }
        if (visited->count(componentId) > 0) {
            return Error() << StringPrintf("XML configuration has duplicated component(%s) in |%s| "
                                           "attribute of |%s| tag",
                                           toString(componentId).c_str(), kAttrId, kTagComponent);
        }
        visited->insert(componentId);
        const char* powerState = pComponent->GetText();
        if (!strcmp(powerState, kPowerStateOn)) {
            policy->enabledComponents.push_back(componentId);
        } else if (!strcmp(powerState, kPowerStateOff)) {
            policy->disabledComponents.push_back(componentId);
        } else {
            return Error() << StringPrintf("XML configuration has invalid value(%s) in |%s| tag",
                                           safePtrPrint(powerState), kTagComponent);
        }
    }
    return {};
}

Result<void> readOtherComponents(const XMLElement* pPolicy, CarPowerPolicyPtr policy,
                                 const std::unordered_set<PowerComponent>& visited) {
    const char* otherComponentBehavior = kPowerStateUntouched;
    const XMLElement* pElement = pPolicy->FirstChildElement(kTagOtherComponents);
    if (pElement != nullptr) {
        if (pElement->QueryStringAttribute(kAttrBehavior, &otherComponentBehavior) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag",
                                           kAttrBehavior, kTagOtherComponents);
        }
    }
    if (!strcmp(otherComponentBehavior, kPowerStateOn)) {
        iterateAllPowerComponents([&visited, &policy](PowerComponent component) -> bool {
            if (visited.count(component) == 0) {
                policy->enabledComponents.push_back(component);
            }
            return true;
        });
    } else if (!strcmp(otherComponentBehavior, kPowerStateOff)) {
        iterateAllPowerComponents([&visited, &policy](PowerComponent component) -> bool {
            if (visited.count(component) == 0) {
                policy->disabledComponents.push_back(component);
            }
            return true;
        });
    } else if (!strcmp(otherComponentBehavior, kPowerStateUntouched)) {
        // Do nothing
    } else {
        return Error() << StringPrintf("XML configuration has invalid value(%s) in |%s| attribute "
                                       "of |%s| tag",
                                       safePtrPrint(otherComponentBehavior), kAttrBehavior,
                                       kTagOtherComponents);
    }
    return {};
}

Result<std::vector<CarPowerPolicyPtr>> readPolicies(const XMLElement* pRoot, const char* tag,
                                                    bool includeOtherComponents) {
    std::vector<CarPowerPolicyPtr> policies;
    const XMLElement* pPolicies = pRoot->FirstChildElement(tag);
    if (pPolicies == nullptr) {
        return std::vector<CarPowerPolicyPtr>();
    }
    for (const XMLElement* pPolicy = pPolicies->FirstChildElement(kTagPolicy); pPolicy != nullptr;
         pPolicy = pPolicy->NextSiblingElement(kTagPolicy)) {
        std::unordered_set<PowerComponent> visited;
        const char* policyId;
        if (pPolicy->QueryStringAttribute(kAttrId, &policyId) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag", kAttrId,
                                           kTagPolicy);
        }
        if (includeOtherComponents && isSystemPowerPolicy(policyId)) {
            return Error() << "Policy ID should not start with \"system_power_policy_\"";
        }
        auto policy = std::make_shared<CarPowerPolicy>();
        policy->policyId = policyId;

        auto ret = readComponents(pPolicy, policy, &visited);
        if (!ret.ok()) {
            return ret.error();
        }
        if (includeOtherComponents) {
            ret = readOtherComponents(pPolicy, policy, visited);
            if (!ret.ok()) {
                return ret.error();
            }
        }
        policies.push_back(policy);
    }
    return policies;
}

Result<PolicyGroup> readPolicyGroup(
        const XMLElement* pPolicyGroup,
        const std::unordered_map<std::string, CarPowerPolicyPtr>& registeredPowerPolicies) {
    PolicyGroup policyGroup;
    for (const XMLElement* pDefaultPolicy = pPolicyGroup->FirstChildElement(kTagDefaultPolicy);
         pDefaultPolicy != nullptr;
         pDefaultPolicy = pDefaultPolicy->NextSiblingElement(kTagDefaultPolicy)) {
        const char* state;
        if (pDefaultPolicy->QueryStringAttribute(kAttrState, &state) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag", kAttrState,
                                           kTagDefaultPolicy);
        }
        int32_t powerState = toVehiclePowerState(state);
        if (!isValidPowerState(powerState)) {
            return Error() << StringPrintf("Target state(%s) is not valid", state);
        }
        const char* policyId;
        if (pDefaultPolicy->QueryStringAttribute(kAttrId, &policyId) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag", kAttrId,
                                           kTagDefaultPolicy);
        }
        if (registeredPowerPolicies.count(policyId) == 0) {
            return Error() << StringPrintf("Policy(id: %s) is not registered", policyId);
        }
        policyGroup.emplace(powerState, policyId);
    }
    for (const XMLElement* pNoPolicy = pPolicyGroup->FirstChildElement(kTagNoDefaultPolicy);
         pNoPolicy != nullptr; pNoPolicy = pNoPolicy->NextSiblingElement(kTagNoDefaultPolicy)) {
        const char* state;
        if (pNoPolicy->QueryStringAttribute(kAttrState, &state) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag", kAttrState,
                                           kTagNoDefaultPolicy);
        }
        int32_t powerState = toVehiclePowerState(state);
        if (!isValidPowerState(powerState)) {
            return Error() << StringPrintf("Target state(%s) is not valid", state);
        }
        if (policyGroup.count(powerState) > 0) {
            return Error()
                    << StringPrintf("Target state(%s) is specified both in |%s| and |%s| tags",
                                    state, kTagDefaultPolicy, kTagNoDefaultPolicy);
        }
    }
    return policyGroup;
}

Result<std::unordered_map<std::string, PolicyGroup>> readPolicyGroups(
        const XMLElement* pRoot,
        const std::unordered_map<std::string, CarPowerPolicyPtr>& registeredPowerPolicies) {
    const XMLElement* pPolicyGroups = pRoot->FirstChildElement(kTagPolicyGroups);
    if (pPolicyGroups == nullptr) {
        return std::unordered_map<std::string, PolicyGroup>();
    }
    std::unordered_map<std::string, PolicyGroup> policyGroups;

    for (const XMLElement* pPolicyGroup = pPolicyGroups->FirstChildElement(kTagPolicyGroup);
         pPolicyGroup != nullptr;
         pPolicyGroup = pPolicyGroup->NextSiblingElement(kTagPolicyGroup)) {
        const char* policyGroupId;
        if (pPolicyGroup->QueryStringAttribute(kAttrId, &policyGroupId) != XML_SUCCESS) {
            return Error() << StringPrintf("Failed to read |%s| attribute in |%s| tag", kAttrId,
                                           kTagPolicyGroup);
        }
        const auto& policyGroup = readPolicyGroup(pPolicyGroup, registeredPowerPolicies);
        if (!policyGroup.ok()) {
            return Error() << policyGroup.error();
        }
        policyGroups.emplace(policyGroupId, *policyGroup);
    }
    return policyGroups;
}

bool isConfigurableComponent(PowerComponent component) {
    return kNoUserInteractionConfigurableComponents.count(component) > 0;
}

Result<void> checkConfigurableComponents(const std::vector<PowerComponent>& components) {
    for (auto component : components) {
        if (!isConfigurableComponent(component)) {
            return Error()
                    << StringPrintf("Component(%s) is not configurable in system power policy.",
                                    toString(component).c_str());
        }
    }
    return {};
}

Result<std::vector<CarPowerPolicyPtr>> readSystemPolicyOverrides(const XMLElement* pRoot) {
    const auto& systemPolicyOverrides = readPolicies(pRoot, kTagSystemPolicyOverrides, false);
    if (!systemPolicyOverrides.ok()) {
        return Error() << systemPolicyOverrides.error().message();
    }
    for (auto policy : *systemPolicyOverrides) {
        if (policy->policyId != kSystemPolicyIdNoUserInteraction) {
            return Error() << StringPrintf("System power policy(%s) is not supported.",
                                           policy->policyId.c_str());
        }
        auto ret = checkConfigurableComponents(policy->enabledComponents);
        if (!ret.ok()) {
            return ret.error();
        }
        ret = checkConfigurableComponents(policy->disabledComponents);
        if (!ret.ok()) {
            return ret.error();
        }
    }
    return systemPolicyOverrides;
}

// configureComponents assumes that previously validated components are passed.
void configureComponents(const std::vector<PowerComponent>& configComponents,
                         std::vector<PowerComponent>* componentsAddedTo,
                         std::vector<PowerComponent>* componentsRemovedFrom) {
    for (const auto component : configComponents) {
        auto it = std::find(componentsAddedTo->begin(), componentsAddedTo->end(), component);
        if (it == componentsAddedTo->end()) {
            componentsAddedTo->push_back(component);
        }
        it = std::find(componentsRemovedFrom->begin(), componentsRemovedFrom->end(), component);
        if (it != componentsRemovedFrom->end()) {
            componentsRemovedFrom->erase(it);
        }
    }
}

Result<void> stringsToComponents(const std::vector<std::string>& arr,
                                 std::vector<PowerComponent>* components) {
    for (const auto& c : arr) {
        const char* component = c.c_str();
        PowerComponent componentId = toPowerComponent(component, "");
        if (componentId == INVALID_POWER_COMPONENT) {
            return Error() << StringPrintf("%s is not a valid component", component);
        }
        components->push_back(componentId);
    }
    return {};
}

CarPowerPolicyPtr createPolicy(const char* policyId,
                               const std::vector<PowerComponent>& enabledComponents,
                               const std::vector<PowerComponent>& disabledComponents) {
    CarPowerPolicyPtr policy = std::make_shared<CarPowerPolicy>();
    policy->policyId = policyId;
    policy->enabledComponents = enabledComponents;
    policy->disabledComponents = disabledComponents;
    return policy;
}

}  // namespace

std::string toString(const std::vector<PowerComponent>& components) {
    size_t size = components.size();
    if (size == 0) {
        return "none";
    }
    std::string filterStr = toString(components[0]);
    for (int i = 1; i < size; i++) {
        StringAppendF(&filterStr, ", %s", toString(components[i]).c_str());
    }
    return filterStr;
}

std::string toString(const CarPowerPolicy& policy) {
    return StringPrintf("%s(enabledComponents: %s, disabledComponents: %s)",
                        policy.policyId.c_str(), toString(policy.enabledComponents).c_str(),
                        toString(policy.disabledComponents).c_str());
}

bool isSystemPowerPolicy(const std::string& policyId) {
    return StartsWith(policyId, kSystemPolicyPrefix);
}

void PolicyManager::init() {
    initRegularPowerPolicy();
    mPolicyGroups.clear();
    initPreemptivePowerPolicy();
    readPowerPolicyConfiguration();
}

Result<CarPowerPolicyMeta> PolicyManager::getPowerPolicy(const std::string& policyId) const {
    if (mRegisteredPowerPolicies.count(policyId) > 0) {
        return CarPowerPolicyMeta{
                .powerPolicy = mRegisteredPowerPolicies.at(policyId),
                .isPreemptive = false,
        };
    }
    if (mPreemptivePowerPolicies.count(policyId) > 0) {
        return CarPowerPolicyMeta{
                .powerPolicy = mPreemptivePowerPolicies.at(policyId),
                .isPreemptive = true,
        };
    }
    return Error() << StringPrintf("Power policy(id: %s) is not found", policyId.c_str());
}

Result<CarPowerPolicyPtr> PolicyManager::getDefaultPowerPolicyForState(
        const std::string& groupId, VehicleApPowerStateReport state) const {
    if (mPolicyGroups.count(groupId) == 0) {
        return Error() << StringPrintf("Power policy group(%s) is not available", groupId.c_str());
    }
    PolicyGroup policyGroup = mPolicyGroups.at(groupId);
    int32_t key = static_cast<int32_t>(state);
    if (policyGroup.count(key) == 0) {
        return Error() << StringPrintf("Policy for %s is not found", toString(state).c_str());
    }
    return mRegisteredPowerPolicies.at(policyGroup.at(key));
}

bool PolicyManager::isPowerPolicyGroupAvailable(const std::string& groupId) const {
    return mPolicyGroups.count(groupId) > 0;
}

bool PolicyManager::isPreemptivePowerPolicy(const std::string& policyId) const {
    return mPreemptivePowerPolicies.count(policyId) > 0;
}

Result<void> PolicyManager::definePowerPolicy(const std::string& policyId,
                                              const std::vector<std::string>& enabledComponents,
                                              const std::vector<std::string>& disabledComponents) {
    if (mRegisteredPowerPolicies.count(policyId) > 0) {
        return Error() << StringPrintf("%s is already registered", policyId.c_str());
    }
    auto policy = std::make_shared<CarPowerPolicy>();
    policy->policyId = policyId;
    auto ret = stringsToComponents(enabledComponents, &policy->enabledComponents);
    if (!ret.ok()) {
        return ret;
    }
    ret = stringsToComponents(disabledComponents, &policy->disabledComponents);
    if (!ret.ok()) {
        return ret;
    }
    mRegisteredPowerPolicies.emplace(policyId, policy);
    return {};
}

Result<void> PolicyManager::dump(int fd, const Vector<String16>& /*args*/) {
    const char* indent = "  ";
    const char* doubleIndent = "    ";
    const char* tripleIndent = "      ";

    WriteStringToFd(StringPrintf("%sRegistered power policies:%s\n", indent,
                                 mRegisteredPowerPolicies.size() ? "" : " none"),
                    fd);
    for (auto& it : mRegisteredPowerPolicies) {
        WriteStringToFd(StringPrintf("%s- %s\n", doubleIndent, toString(*it.second).c_str()), fd);
    }
    WriteStringToFd(StringPrintf("%sPower policy groups:%s\n", indent,
                                 mPolicyGroups.size() ? "" : " none"),
                    fd);
    for (auto& itGroup : mPolicyGroups) {
        WriteStringToFd(StringPrintf("%s%s\n", doubleIndent, itGroup.first.c_str()), fd);
        for (auto& itMapping : itGroup.second) {
            VehicleApPowerStateReport state =
                    static_cast<VehicleApPowerStateReport>(itMapping.first);
            WriteStringToFd(StringPrintf("%s- %s --> %s\n", tripleIndent, toString(state).c_str(),
                                         itMapping.second.c_str()),
                            fd);
        }
    }
    WriteStringToFd(StringPrintf("%sNo user interaction power policy: %s\n", indent,
                                 toString(*mPreemptivePowerPolicies.at(
                                                  kSystemPolicyIdNoUserInteraction))
                                         .c_str()),
                    fd);
    return {};
}

void PolicyManager::readPowerPolicyConfiguration() {
    XMLDocument xmlDoc;
    xmlDoc.LoadFile(kVendorPolicyFile);
    if (xmlDoc.ErrorID() != XML_SUCCESS) {
        logXmlError(StringPrintf("Failed to read and/or parse %s", kVendorPolicyFile));
        return;
    }
    readPowerPolicyFromXml(xmlDoc);
}

void PolicyManager::readPowerPolicyFromXml(const XMLDocument& xmlDoc) {
    const XMLElement* pRootElement = xmlDoc.RootElement();
    if (!pRootElement || strcmp(pRootElement->Name(), kTagRoot)) {
        logXmlError(StringPrintf("XML file is not in the required format"));
        return;
    }
    const auto& registeredPolicies = readPolicies(pRootElement, kTagPolicies, true);
    if (!registeredPolicies.ok()) {
        logXmlError(StringPrintf("Reading policies failed: %s",
                                 registeredPolicies.error().message().c_str()));
        return;
    }
    std::unordered_map<std::string, CarPowerPolicyPtr> registeredPoliciesMap;
    for (auto policy : *registeredPolicies) {
        registeredPoliciesMap.emplace(policy->policyId, policy);
    }
    const auto& policyGroups = readPolicyGroups(pRootElement, registeredPoliciesMap);
    if (!policyGroups.ok()) {
        logXmlError(StringPrintf("Reading power policy groups for power state failed: %s",
                                 policyGroups.error().message().c_str()));
        return;
    }
    const auto& systemPolicyOverrides = readSystemPolicyOverrides(pRootElement);
    if (!systemPolicyOverrides.ok()) {
        logXmlError(StringPrintf("Reading system power policy overrides failed: %s",
                                 systemPolicyOverrides.error().message().c_str()));
        return;
    }

    mRegisteredPowerPolicies = registeredPoliciesMap;
    mPolicyGroups = *policyGroups;
    reconstructNoUserInteractionPolicy(*systemPolicyOverrides);
}

void PolicyManager::reconstructNoUserInteractionPolicy(
        const std::vector<CarPowerPolicyPtr>& policyOverrides) {
    CarPowerPolicyPtr systemPolicy = mPreemptivePowerPolicies.at(kSystemPolicyIdNoUserInteraction);
    for (auto policy : policyOverrides) {
        configureComponents(policy->enabledComponents, &systemPolicy->enabledComponents,
                            &systemPolicy->disabledComponents);
        configureComponents(policy->disabledComponents, &systemPolicy->disabledComponents,
                            &systemPolicy->enabledComponents);
    }
}

void PolicyManager::initRegularPowerPolicy() {
    mRegisteredPowerPolicies.clear();
    mRegisteredPowerPolicies.emplace(kSystemPolicyIdAllOn,
                                     createPolicy(kSystemPolicyIdAllOn, kAllComponents,
                                                  kNoComponents));

    std::vector<PowerComponent> initialOnDisabledComponents;
    for (const auto component : enum_range<PowerComponent>()) {
        if (std::find(kInitialOnComponents.begin(), kInitialOnComponents.end(), component) ==
            kInitialOnComponents.end()) {
            initialOnDisabledComponents.push_back(component);
        }
    }
    mRegisteredPowerPolicies.emplace(kSystemPolicyIdInitialOn,
                                     createPolicy(kSystemPolicyIdInitialOn, kInitialOnComponents,
                                                  initialOnDisabledComponents));
}

void PolicyManager::initPreemptivePowerPolicy() {
    mPreemptivePowerPolicies.clear();
    mPreemptivePowerPolicies.emplace(kSystemPolicyIdNoUserInteraction,
                                     createPolicy(kSystemPolicyIdNoUserInteraction,
                                                  kNoUserInteractionEnabledComponents,
                                                  kNoUserInteractionDisabledComponents));
    mPreemptivePowerPolicies.emplace(kSystemPolicyIdSuspendToRam,
                                     createPolicy(kSystemPolicyIdSuspendToRam, kNoComponents,
                                                  kSuspendToRamDisabledComponents));
}

}  // namespace powerpolicy
}  // namespace automotive
}  // namespace frameworks
}  // namespace android
