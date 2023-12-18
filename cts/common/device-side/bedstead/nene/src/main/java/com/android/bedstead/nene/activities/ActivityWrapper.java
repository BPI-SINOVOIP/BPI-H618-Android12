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

package com.android.bedstead.nene.activities;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

class ActivityWrapper implements NeneActivity {

    private final Activity mActivity;

    ActivityWrapper(Activity activity) {
        mActivity = activity;
    }

    @Override
    public void startLockTask() {
        mActivity.startLockTask();
    }

    @Override
    public void stopLockTask() {
        mActivity.stopLockTask();
    }

    @Override
    public void finish() {
        mActivity.finish();
    }

    @Override
    public boolean isFinishing() {
        return mActivity.isFinishing();
    }

    @Override
    public void startActivity(Intent intent) {
        mActivity.startActivity(intent);
    }

    @Override
    public void startActivity(Intent intent, Bundle options) {
        mActivity.startActivity(intent, options);
    }
}
