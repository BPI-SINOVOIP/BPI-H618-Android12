/*
 * Copyright (C) 2021 The Android Open Source Project
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

#include <string>

#include <netdutils/InternetAddresses.h>

#include "PrivateDnsCommon.h"

namespace android::net {

class IPrivateDnsServer {
  public:
    virtual ~IPrivateDnsServer(){};

    virtual PrivateDnsTransport transport() const = 0;
    bool isDot() const { return transport() == PrivateDnsTransport::kDot; }
    bool isDoh() const { return transport() == PrivateDnsTransport::kDoh; }

    // Returns the provider name of the server.
    virtual std::string provider() const = 0;

    // Returns the IP address of the server.
    virtual netdutils::IPSockAddr addr() const = 0;

    // Returns the socket mark used for probe.
    virtual uint32_t validationMark() const = 0;

    // Sets the validation state.
    virtual void setValidationState(Validation val) = 0;

    // Returns the validation state.
    virtual Validation validationState() const = 0;

    // Checks the server supports private DNS.
    virtual bool probe() = 0;

    // Sets if the server should be active.
    virtual void setActive(bool val) = 0;

    // Returns if the server is active.
    virtual bool active() const = 0;
};

}  // namespace android::net
