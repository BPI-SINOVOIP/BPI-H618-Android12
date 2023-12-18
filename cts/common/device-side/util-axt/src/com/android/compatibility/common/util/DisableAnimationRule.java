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

package com.android.compatibility.common.util;

import static com.android.compatibility.common.util.SystemUtil.runShellCommand;

import androidx.annotation.NonNull;

import org.junit.runner.Description;
import org.junit.runners.model.Statement;

public class DisableAnimationRule extends BeforeAfterRule {
    @NonNull
    private final GlobalSetting mWindowAnimationScaleSetting = new GlobalSetting(
            "window_animation_scale");
    @NonNull
    private final GlobalSetting mTransitionAnimationScaleSetting = new GlobalSetting(
            "transition_animation_scale");
    @NonNull
    private final GlobalSetting mAnimatorDurationScaleSetting = new GlobalSetting(
            "animator_duration_scale");

    @Override
    protected void onBefore(Statement base, Description description) throws Throwable {
        mWindowAnimationScaleSetting.put("0");
        mTransitionAnimationScaleSetting.put("0");
        mAnimatorDurationScaleSetting.put("0");
    }

    @Override
    protected void onAfter(Statement base, Description description) throws Throwable {
        mWindowAnimationScaleSetting.restore();
        mTransitionAnimationScaleSetting.restore();
        mAnimatorDurationScaleSetting.restore();
    }

    private static class GlobalSetting {
        @NonNull
        private final String mName;

        private String mInitialValue;

        public GlobalSetting(@NonNull String name) {
            mName = name;
        }

        public void put(@NonNull String value) {
            mInitialValue = runShellCommand("settings get global " + mName);
            runShellCommand("settings put global " + mName + " " + value);
        }

        public void restore() {
            runShellCommand("settings put global " + mName + " " + mInitialValue);
        }
    }
}
