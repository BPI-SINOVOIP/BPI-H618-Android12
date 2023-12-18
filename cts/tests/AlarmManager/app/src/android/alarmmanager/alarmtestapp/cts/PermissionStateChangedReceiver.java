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

package android.alarmmanager.alarmtestapp.cts;

import android.app.AlarmManager;
import android.app.ForegroundServiceStartNotAllowedException;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.util.Log;

/**
 * Receives ACTION_SCHEDULE_EXACT_ALARM_PERMISSION_STATE_CHANGED, and starts and FGS.
 * Send whether it was able to start the FGS to the main test app using a broadcast.
 */
public class PermissionStateChangedReceiver extends BroadcastReceiver {
    private static final String TAG = PermissionStateChangedReceiver.class.getSimpleName();
    private static final String PACKAGE_NAME = "android.alarmmanager.alarmtestapp.cts";

    private static final String MAIN_CTS_PACKAGE = "android.alarmmanager.cts";

    public static String ACTION_FGS_START_RESULT = PACKAGE_NAME + ".action.FGS_START_RESULT";
    public static String EXTRA_FGS_START_RESULT = PACKAGE_NAME + ".extra.FGS_START_RESULT";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.d(TAG, "Broadcast received: " + intent);
        if (AlarmManager.ACTION_SCHEDULE_EXACT_ALARM_PERMISSION_STATE_CHANGED
                .equals(intent.getAction())) {
            tryStartingFgs(context);
        }
    }

    private void tryStartingFgs(Context context) {
        String result = "Unknown failure";
        try {
            // Try starting a foreground service.
            Intent i = new Intent(context, TestService.class);
            context.startForegroundService(i);

            result = ""; // Indicates success
        } catch (ForegroundServiceStartNotAllowedException e) {
            result = "ForegroundServiceStartNotAllowedException was thrown";
        } catch (Exception e) {
            result = "Unexpected exception was thrown: " + e;
        }

        // Send the result broadcast to the main CTS package.
        Intent response = new Intent(ACTION_FGS_START_RESULT);
        response.setPackage(MAIN_CTS_PACKAGE);
        response.putExtra(EXTRA_FGS_START_RESULT, result);

        Log.d(TAG, "Sending response: " + result);
        context.sendBroadcast(response);
    }
}