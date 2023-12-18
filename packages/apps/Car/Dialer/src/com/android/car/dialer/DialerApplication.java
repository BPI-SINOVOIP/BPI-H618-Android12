/*
 * Copyright (C) 2018 The Android Open Source Project
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

package com.android.car.dialer;

import android.app.Application;
import android.bluetooth.BluetoothDevice;

import androidx.lifecycle.LiveData;

import com.android.car.dialer.bluetooth.CallHistoryManager;
import com.android.car.dialer.bluetooth.UiBluetoothMonitor;
import com.android.car.dialer.framework.AndroidFramework;
import com.android.car.dialer.notification.MissedCallNotificationController;
import com.android.car.dialer.telecom.UiCallManager;
import com.android.car.telephony.common.InMemoryPhoneBook;

import javax.inject.Inject;
import javax.inject.Named;

import dagger.hilt.android.HiltAndroidApp;

/** Application for Dialer app. */
@HiltAndroidApp(Application.class)
public final class DialerApplication extends Hilt_DialerApplication {
    // Explicit injection for components that need to init on application create.
    @Inject
    UiCallManager mUiCallManager;
    @Inject
    UiBluetoothMonitor mUiBluetoothMonitor;
    @Inject @Named("Hfp")
    LiveData<BluetoothDevice> mCurrentHfpDeviceLiveData;
    @Inject
    CallHistoryManager mCallHistoryManager;
    @Inject
    MissedCallNotificationController mMissedCallNotificationController;

    @Inject
    AndroidFramework mAndroidFramework;

    @Override
    public void onCreate() {
        super.onCreate();
        mAndroidFramework.start();
        InMemoryPhoneBook.init(this);
    }
}
