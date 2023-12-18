/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.softwinner.screenrecord;

import android.app.Service;
import android.content.Intent;
import android.os.IBinder;

public class TakeScreenrecordService extends Service {
    private static final String TAG = "TakeScreenrecordService";

    private static GlobalScreenrecord mScreenrecord;

    @Override
    public IBinder onBind(Intent intent) {
        return null;
    }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        boolean updateState = intent == null || intent.getBooleanExtra(GlobalScreenrecord.EXTRA_SCREENRECORD_UPDATE_STATE, false);
        if (updateState) {
            mScreenrecord.updateState();
        } else {
            mScreenrecord.autoScreenrecord();
        }
        return START_NOT_STICKY;
    }

    @Override
    public void onCreate() {
        super.onCreate();
        if (mScreenrecord == null) {
            mScreenrecord = new GlobalScreenrecord(TakeScreenrecordService.this);
        }
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        mScreenrecord = null;
    }

}
