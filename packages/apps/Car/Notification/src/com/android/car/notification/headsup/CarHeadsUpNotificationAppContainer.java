/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.car.notification.headsup;

import android.content.Context;
import android.graphics.PixelFormat;
import android.view.Gravity;
import android.view.WindowManager;

/**
 * A controller for Notification application's HUN display.
 *
 * Used to attach HUNs views to window.
 */
public class CarHeadsUpNotificationAppContainer extends CarHeadsUpNotificationContainer {
    private static final String TAG = "CarHUNContainerApp";

    public CarHeadsUpNotificationAppContainer(Context context) {
        super(context, context.getSystemService(WindowManager.class));
    }

    @Override
    protected WindowManager.LayoutParams getWindowManagerLayoutParams() {
        WindowManager.LayoutParams wrapperParams = new WindowManager.LayoutParams(
                WindowManager.LayoutParams.MATCH_PARENT,
                WindowManager.LayoutParams.MATCH_PARENT,
                // This type allows covering status bar and receiving touch input
                WindowManager.LayoutParams.TYPE_SYSTEM_ERROR,
                WindowManager.LayoutParams.FLAG_LAYOUT_IN_SCREEN
                        | WindowManager.LayoutParams.FLAG_NOT_FOCUSABLE,
                PixelFormat.TRANSLUCENT);
        wrapperParams.gravity = getShowHunOnBottom() ? Gravity.BOTTOM : Gravity.TOP;
        return wrapperParams;
    }
}