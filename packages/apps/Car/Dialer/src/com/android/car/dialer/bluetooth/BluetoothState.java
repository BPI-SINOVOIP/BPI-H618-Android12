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

package com.android.car.dialer.bluetooth;

import androidx.annotation.IntDef;

@IntDef({
        BluetoothState.UNKNOWN,
        BluetoothState.DISABLED,
        BluetoothState.ENABLED,
})
public @interface BluetoothState {
    /** Bluetooth is not supported on the current device */
    int UNKNOWN = 0;
    /** Bluetooth is disabled */
    int DISABLED = 1;
    /** Bluetooth is enabled */
    int ENABLED = 2;
}
