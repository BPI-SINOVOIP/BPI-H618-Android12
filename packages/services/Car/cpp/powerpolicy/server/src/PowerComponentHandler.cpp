/*
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
#define DEBUG false

#include "PowerComponentHandler.h"

#include <android-base/file.h>
#include <android-base/stringprintf.h>
#include <binder/Enums.h>

namespace android {
namespace frameworks {
namespace automotive {
namespace powerpolicy {

using ::android::base::Error;
using ::android::base::Result;
using ::android::base::StringPrintf;
using ::android::base::WriteStringToFd;

void PowerComponentHandler::init() {
    Mutex::Autolock lock(mMutex);
    mAccumulatedPolicy = std::make_shared<CarPowerPolicy>();
    for (const auto componentId : enum_range<PowerComponent>()) {
        mAccumulatedPolicy->disabledComponents.push_back(componentId);
    }
}

void PowerComponentHandler::applyPowerPolicy(const CarPowerPolicyPtr& powerPolicy) {
    Mutex::Autolock lock(mMutex);
    std::unordered_map<PowerComponent, bool> componentStates;
    mAccumulatedPolicy->policyId = powerPolicy->policyId;
    for (const auto component : mAccumulatedPolicy->enabledComponents) {
        componentStates[component] = true;
    }
    for (const auto component : mAccumulatedPolicy->disabledComponents) {
        componentStates[component] = false;
    }
    for (const auto component : powerPolicy->enabledComponents) {
        componentStates[component] = true;
    }
    for (const auto component : powerPolicy->disabledComponents) {
        componentStates[component] = false;
    }
    mAccumulatedPolicy->enabledComponents.clear();
    mAccumulatedPolicy->disabledComponents.clear();
    for (const auto [component, state] : componentStates) {
        if (state) {
            mAccumulatedPolicy->enabledComponents.push_back(component);
        } else {
            mAccumulatedPolicy->disabledComponents.push_back(component);
        }
    }
}

Result<bool> PowerComponentHandler::getPowerComponentState(const PowerComponent componentId) const {
    Mutex::Autolock lock(mMutex);
    auto findComponent = [componentId](const std::vector<PowerComponent>& components) -> bool {
        return std::find(components.begin(), components.end(), componentId) != components.end();
    };
    if (findComponent(mAccumulatedPolicy->enabledComponents)) {
        return true;
    }
    if (findComponent(mAccumulatedPolicy->disabledComponents)) {
        return false;
    }
    return Error() << StringPrintf("Invalid power component(%d)", componentId);
}

CarPowerPolicyPtr PowerComponentHandler::getAccumulatedPolicy() const {
    Mutex::Autolock lock(mMutex);
    return mAccumulatedPolicy;
}

Result<void> PowerComponentHandler::dump(int fd) {
    Mutex::Autolock lock(mMutex);
    const char* indent = "  ";
    const char* doubleIndent = "    ";
    auto printComponents = [fd](const std::vector<PowerComponent>& components) {
        bool isNotFirst = false;
        for (const auto component : components) {
            if (isNotFirst) {
                WriteStringToFd(", ", fd);
            } else {
                isNotFirst = true;
            }
            WriteStringToFd(toString(component), fd);
        }
        WriteStringToFd("\n", fd);
    };

    WriteStringToFd(StringPrintf("%sCurrent state of power components:\n", indent), fd);
    WriteStringToFd(StringPrintf("%sEnabled components: ", doubleIndent), fd);
    printComponents(mAccumulatedPolicy->enabledComponents);
    WriteStringToFd(StringPrintf("%sDisabled components: ", doubleIndent), fd);
    printComponents(mAccumulatedPolicy->disabledComponents);

    return {};
}

}  // namespace powerpolicy
}  // namespace automotive
}  // namespace frameworks
}  // namespace android
