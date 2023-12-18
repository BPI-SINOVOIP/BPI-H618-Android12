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

#include "DisplayConfig.h"
#include "DisplayConfigImpl.h"
#include "DisplayConfigService.h"
#include "IHWCPrivateService.h"

#if defined(_platform_homlet_)
#include "HomletDisplayConfig.h"
#else
#include "TabletDisplayConfig.h"
#endif

namespace sunxi {

using ::vendor::display::config::V1_0::implementation::DisplayConfig;

DisplayConfigImpl* createDisplayConfigImplByPlatform(IHWCPrivateService& client)
{
#if   defined(_platform_tablet_)
    return new TabletDisplayConfig(client);
#elif defined(_platform_homlet_)
    return new HomletDisplayConfig(client);
#else
    // use TabletDisplayConfig for any other platform by default
    return new TabletDisplayConfig(client);
#endif
}

DisplayConfigService::DisplayConfigService(IHWCPrivateService& client)
{
    mDisplayConfigImpl = createDisplayConfigImplByPlatform(client);
    mDisplayConfig = new DisplayConfig(mDisplayConfigImpl);
}

android::status_t DisplayConfigService::publish()
{
    android::status_t status = mDisplayConfig->registerAsService();
    ALOGD("IDisplayConfig register: %s", (status == android::OK) ? "ok" : "error");
    return status;
}

} // namespace sunxi
