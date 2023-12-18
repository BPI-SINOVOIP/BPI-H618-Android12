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

#ifndef __DEVICE_TABLE_H__
#define __DEVICE_TABLE_H__

#include <utils/Flattenable.h>
#include <utils/Vector.h>

namespace sunxi {

class DeviceTable : public android::LightFlattenable<DeviceTable> {
public:
    struct device_info {
        /*
         * logicalId:
         *   0 - primary  display.
         *   1 - external display.
         */
        int logicalId;
        int type;
        int mode;

        /*
         * The specific device state.
         *   0 - closed, hwc should plugout this device.
         *   1 - opened, hwc should plugin and commit layers.
         */
        int enabled;

        device_info(int id, int t, int m, int en)
          : logicalId(id),
            type(t),
            mode(m),
            enabled(en) { }
        device_info() { }
    };

    typedef struct device_info device_info_t;
    android::Vector<device_info_t> mTables;

    bool isFixedSize() const;
    size_t getFlattenedSize() const;
    android::status_t flatten(void *buffer, size_t size) const;
    android::status_t unflatten(void const *buffer, size_t size);

    DeviceTable() : mTables() {};
};

} // namespace sunxi
#endif
