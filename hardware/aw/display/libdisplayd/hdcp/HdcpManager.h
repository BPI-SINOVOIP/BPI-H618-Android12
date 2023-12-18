/*
 * Copyright (C) 2017 The Android Open Source Project
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

#ifndef HDCP_MANAGER_H
#define HDCP_MANAGER_H

class HdcpManager {
public:
    HdcpManager();
   ~HdcpManager() = default;

    enum HdcpAuthorizedStatus {
        ERROR,         // hdp authentication error
        UN_AUTHORIZED,
        AUTHORIZED,
    };

    enum HdcpLevel : uint32_t {
        HDCP_UNKNOWN,  // Unable to determine the HDCP level
        HDCP_NONE,     // No HDCP, output is unprotected
        HDCP_V1,       // HDCP version 1.0
        HDCP_V2,       // HDCP version 2.0 Type 1.
        HDCP_V2_1,     // HDCP version 2.1 Type 1.
        HDCP_V2_2,     // HDCP version 2.2 Type 1.
        HDCP_NO_OUTPUT // No digital output, implicitly secure
    };

    HdcpLevel getConnectedHdcpLevel() const;
    HdcpAuthorizedStatus getAuthorizedStatus() const;
    int configHdcp(bool enable);

private:
    int loadFirmware(const char *fw);
    bool mFirmwareReady;
    bool mHdcpEnabled;
};

#endif
