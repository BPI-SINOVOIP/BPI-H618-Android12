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

package android.platform.test.rule.flicker;

import android.os.Environment;
import android.util.Log;

import com.android.server.wm.traces.common.windowmanager.WindowManagerTrace;

/**
 * Base class that encapsulates the logic for enabling the window manager trace, parsing the
 * window manager trace. Extend this class to add validation for window manager trace based
 * flicker conditions.
 */
public abstract class WindowManagerFlickerRuleBase extends FlickerRuleBase {

    private static final String TAG = WindowManagerFlickerRuleBase.class.getSimpleName();

    WindowManagerFlickerRuleBase() {
        enableWmTrace();
        Log.v(TAG, "Enabled the window manager trace.");
    }

    protected void validateFlickerConditions() {
        validateWMFlickerConditions(getWindowManagerTrace());
    }

    /**
     * Override this method to provide window manager trace based flicker validations.
     *
     * @param windowManagerTrace
     */
    protected abstract void validateWMFlickerConditions(WindowManagerTrace windowManagerTrace);
}
