/*
 * Copyright (c) 2021, The Android Open Source Project
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

#include "PowerComponentHandler.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <tuple>
#include <vector>

namespace android {
namespace frameworks {
namespace automotive {
namespace powerpolicy {

using ::testing::UnorderedElementsAreArray;

namespace {

CarPowerPolicyPtr createPolicy(const std::string& policyId,
                               const std::vector<PowerComponent>& enabledComponents,
                               const std::vector<PowerComponent>& disabledComponents) {
    CarPowerPolicyPtr policy = std::make_shared<CarPowerPolicy>();
    policy->policyId = policyId;
    policy->enabledComponents = enabledComponents;
    policy->disabledComponents = disabledComponents;
    return policy;
}

void assertEqual(const CarPowerPolicyPtr& left, const CarPowerPolicyPtr& right) {
    ASSERT_EQ(left->policyId, right->policyId);
    EXPECT_THAT(left->enabledComponents, UnorderedElementsAreArray(right->enabledComponents));
    EXPECT_THAT(left->disabledComponents, UnorderedElementsAreArray(right->disabledComponents));
}

}  // namespace

class PowerComponentHandlerTest : public ::testing::Test {
public:
    PowerComponentHandlerTest() { handler.init(); }

    PowerComponentHandler handler;
};

TEST_F(PowerComponentHandlerTest, TestInitialPowerComponentStates) {
    CarPowerPolicyPtr policy = handler.getAccumulatedPolicy();
    std::vector<PowerComponent> allComponents;
    for (auto componentId : enum_range<PowerComponent>()) {
        allComponents.push_back(componentId);
    }

    EXPECT_THAT(allComponents, UnorderedElementsAreArray(policy->disabledComponents));
}

TEST_F(PowerComponentHandlerTest, TestGetPowerComponentState) {
    CarPowerPolicyPtr policy =
            createPolicy("test_policy", {PowerComponent::WIFI, PowerComponent::NFC},
                         {PowerComponent::AUDIO, PowerComponent::DISPLAY});
    handler.applyPowerPolicy(policy);

    ASSERT_TRUE(*handler.getPowerComponentState(PowerComponent::WIFI));
    ASSERT_TRUE(*handler.getPowerComponentState(PowerComponent::NFC));
    ASSERT_FALSE(*handler.getPowerComponentState(PowerComponent::AUDIO));
    ASSERT_FALSE(*handler.getPowerComponentState(PowerComponent::DISPLAY));
}

TEST_F(PowerComponentHandlerTest, TestApplyPowerPolicy_multipleTimes) {
    std::vector<std::tuple<std::string, std::vector<PowerComponent>, std::vector<PowerComponent>>>
            testCases = {
                    {"test_policy1", {PowerComponent::WIFI}, {PowerComponent::AUDIO}},
                    {"test_policy2",
                     {PowerComponent::WIFI, PowerComponent::DISPLAY},
                     {PowerComponent::NFC}},
                    {"test_policy3",
                     {PowerComponent::CPU, PowerComponent::INPUT},
                     {PowerComponent::WIFI}},
                    {"test_policy4", {PowerComponent::MEDIA, PowerComponent::AUDIO}, {}},
            };
    CarPowerPolicyPtr expectedPolicy =
            createPolicy("test_policy4",
                         {PowerComponent::AUDIO, PowerComponent::MEDIA, PowerComponent::DISPLAY,
                          PowerComponent::INPUT, PowerComponent::CPU},
                         {PowerComponent::BLUETOOTH, PowerComponent::WIFI, PowerComponent::CELLULAR,
                          PowerComponent::ETHERNET, PowerComponent::PROJECTION, PowerComponent::NFC,
                          PowerComponent::VOICE_INTERACTION, PowerComponent::VISUAL_INTERACTION,
                          PowerComponent::TRUSTED_DEVICE_DETECTION, PowerComponent::LOCATION,
                          PowerComponent::MICROPHONE});

    for (const auto& tc : testCases) {
        auto [policyId, enabledComponents, disabledComponents] = tc;
        CarPowerPolicyPtr policy = createPolicy(policyId, enabledComponents, disabledComponents);
        handler.applyPowerPolicy(policy);
    }

    ASSERT_NO_FATAL_FAILURE(assertEqual(expectedPolicy, handler.getAccumulatedPolicy()));
}

}  // namespace powerpolicy
}  // namespace automotive
}  // namespace frameworks
}  // namespace android
