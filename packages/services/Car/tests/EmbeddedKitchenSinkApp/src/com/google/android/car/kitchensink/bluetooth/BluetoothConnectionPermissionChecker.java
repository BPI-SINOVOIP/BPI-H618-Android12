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

package com.google.android.car.kitchensink.bluetooth;

import android.Manifest;
import android.annotation.Nullable;
import android.app.Activity;
import android.content.pm.PackageManager;

import androidx.activity.result.contract.ActivityResultContracts;
import androidx.fragment.app.Fragment;

public final class BluetoothConnectionPermissionChecker {
    private BluetoothConnectionPermissionChecker() {
    }

    static boolean isPermissionGranted(Activity mActivity) {
        return mActivity.checkSelfPermission(Manifest.permission.BLUETOOTH_CONNECT)
                == PackageManager.PERMISSION_GRANTED;
    }

    static void requestPermission(Fragment fragment, @Nullable Runnable isGrantedRunnable,
            @Nullable Runnable isNotGrantedRunnable) {
        fragment.registerForActivityResult(new ActivityResultContracts.RequestPermission(),
                isGranted -> {
                    if (isGranted) {
                        if (isGrantedRunnable != null) {
                            isGrantedRunnable.run();
                        }
                    } else {
                        if (isNotGrantedRunnable != null) {
                            isNotGrantedRunnable.run();
                        }
                    }
                }).launch(Manifest.permission.BLUETOOTH_CONNECT);
    }
}
