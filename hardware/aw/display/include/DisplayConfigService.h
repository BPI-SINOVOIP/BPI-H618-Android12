/*
 * Copyright (C) 2019 The Android Open Source Project
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

#ifndef _DISPLAY_CONFIG_SERVICE_H_
#define _DISPLAY_CONFIG_SERVICE_H_

#include "DisplayConfig.h"
#include <utils/RefBase.h>

namespace sunxi {

using ::vendor::display::config::V1_0::implementation::DisplayConfig;

class DisplayConfigImpl;
class IHWCPrivateService;

class DisplayConfigService: public android::RefBase {
public:
    DisplayConfigService(IHWCPrivateService& client);
    android::status_t publish();

private:
    // HIDL service instance of IDisplayConfig
    android::sp<DisplayConfig> mDisplayConfig;

    // The mDisplayConfigImpl below run in the composer pocess
    // and wherever necessary do a direct call to HWC to get/set data.
    android::sp<DisplayConfigImpl> mDisplayConfigImpl;
};

} // namespace sunxi
#endif
