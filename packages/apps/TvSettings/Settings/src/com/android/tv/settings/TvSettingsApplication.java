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

package com.android.tv.settings;

import android.app.Application;

/**
 * Application class that instantiates system sound player singleton so sound effects are only
 * loaded once and shared between components.
 */
public class TvSettingsApplication extends Application {
    private SystemSoundsPlayer mSystemSoundsPlayer;

    @Override
    public void onCreate() {
        super.onCreate();
        mSystemSoundsPlayer = new SystemSoundsPlayer(this);
    }

    public SystemSoundsPlayer getSystemSoundsPlayer() {
        return mSystemSoundsPlayer;
    }
}
