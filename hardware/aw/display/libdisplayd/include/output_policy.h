/*
 * Copyright (C) 2020 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <memory>
#include <mutex>
#include <vector>
#include <unordered_map>

#include "platform.h"

struct OutputDeviceInfo {
    int logicalId;
    int type;
    int enabled;
    int connected;
};

class OutputPolicy {
public:
    virtual ~OutputPolicy() = default;
    virtual void updateConnectState(int type, int connected) = 0;
    virtual void getOutputDeviceMapping(std::vector<OutputDeviceInfo>& out) = 0;
    virtual std::string getDebugName() const; 
};

class DefaultOutputPolicy: public OutputPolicy {
public:
    DefaultOutputPolicy(const std::vector<OutputDeviceInfo>& perfectMapping);
    void updateConnectState(int type, int connected);
    void getOutputDeviceMapping(std::vector<OutputDeviceInfo>& out);
    std::string getDebugName() const; 

private:
    void initOutputPolicy();
    void initPerfectMapping(
            const std::vector<OutputDeviceInfo>& perfectMapping);
    void rebuildDisplayMappingLocked();
    void buildSingleOutput();
    void buildDualOutput();
    int getConnectState(int type);

    enum { eSingleOutput = 1, eDualOutput = 2, };

    /* Display id sync from hwcomposer_defs.h */
    enum {
        ePrimaryDisplayId  = 0,
        eExternalDisplayId = 1,
    };

    std::mutex mLock;
    int mPolicy;

    /* logicalId(key) to device info(value) mapping */
    using DisplayMapping = std::unordered_map<int, OutputDeviceInfo>;

    DisplayMapping mCurrentMapping;
    DisplayMapping mPerfectMapping;
};

std::unique_ptr<OutputPolicy> createOutputPolicy(
        const std::vector<OutputDeviceInfo>& perfectMapping);

