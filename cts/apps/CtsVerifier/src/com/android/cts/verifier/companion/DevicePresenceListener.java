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

package com.android.cts.verifier.companion;

import android.companion.CompanionDeviceService;
import android.widget.Toast;

/**
 * A Listener for {@Link CompanionDeviceAwakeTestActivity}
 */
public class DevicePresenceListener extends CompanionDeviceService {
    public static Boolean sDeviceNearBy;

    @Override
    public void onDeviceAppeared(String address) {
        sDeviceNearBy = true;
        Toast.makeText(this, "Device appeared: " + address,
                Toast.LENGTH_LONG).show();
    }

    @Override
    public void onDeviceDisappeared(String address) {
        sDeviceNearBy = false;
        Toast.makeText(this, "Device disappeared: " + address,
                Toast.LENGTH_LONG).show();
    }

}
