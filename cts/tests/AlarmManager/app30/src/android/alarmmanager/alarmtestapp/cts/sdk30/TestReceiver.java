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

package android.alarmmanager.alarmtestapp.cts.sdk30;

import android.app.Activity;
import android.app.AlarmManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * This receiver is to be used to communicate with tests in {@link android.alarmmanager.cts}
 */
public class TestReceiver extends BroadcastReceiver {
    private static final String TAG = TestReceiver.class.getSimpleName();
    public static final String PACKAGE_NAME = "android.alarmmanager.alarmtestapp.cts.sdk30";

    public static final String ACTION_GET_CAN_SCHEDULE_EXACT_ALARM =
            PACKAGE_NAME + ".action.GET_CAN_SCHEDULE_EXACT_ALARM";

    @Override
    public void onReceive(Context context, Intent intent) {
        final AlarmManager am = context.getSystemService(AlarmManager.class);
        switch (intent.getAction()) {
            case ACTION_GET_CAN_SCHEDULE_EXACT_ALARM:
                final boolean result = am.canScheduleExactAlarms();
                setResult(Activity.RESULT_OK, String.valueOf(result), null);
                break;
            default:
                Log.e(TAG, "Unspecified action " + intent.getAction());
                setResult(Activity.RESULT_CANCELED, null, null);
                break;
        }
    }
}