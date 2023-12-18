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

package com.android.tv.settings.bluetooth;

import android.content.Context;
import android.content.Intent;
import androidx.preference.Preference;

import com.android.settingslib.core.AbstractPreferenceController;

/**
 * Controller that shows received files
 */
public class BluetoothFilesPreferenceController extends AbstractPreferenceController {
    private static final String TAG = "BluetoothFilesPrefCtrl";

    public static final String KEY_RECEIVED_FILES = "bt_received_files";

    /* Private intent to show the list of received files */
    static final String ACTION_OPEN_FILES = "com.android.bluetooth.action.TransferHistory";
    static final String EXTRA_SHOW_ALL_FILES = "android.btopp.intent.extra.SHOW_ALL";
    static final String EXTRA_DIRECTION = "direction";


    public BluetoothFilesPreferenceController(Context context) {
        super(context);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_RECEIVED_FILES;
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (KEY_RECEIVED_FILES.equals(preference.getKey())) {
            Intent intent = new Intent(ACTION_OPEN_FILES);
            intent.setFlags(Intent.FLAG_ACTIVITY_NEW_TASK | Intent.FLAG_ACTIVITY_CLEAR_TOP);
            intent.putExtra(EXTRA_DIRECTION, 1 /* DIRECTION_INBOUND */);
            intent.putExtra(EXTRA_SHOW_ALL_FILES, true);
            mContext.startActivity(intent);
            return true;
        }

        return false;
    }


}
