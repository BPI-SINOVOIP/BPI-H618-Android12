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

package com.android.car.ui.toolbar;

import androidx.annotation.Nullable;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.ProgressBarControllerOEMV1;

class ProgressBarControllerAdapterV1 implements ProgressBarController {

    @Nullable
    private final ProgressBarControllerOEMV1 mOemProgressbar;
    private boolean mVisible = false;
    private boolean mIndeterminate = true;
    private int mMax = 100;
    private int mMin = 0;
    private int mProgress = 0;

    ProgressBarControllerAdapterV1(@Nullable ProgressBarControllerOEMV1 oemProgressbar) {
        mOemProgressbar = oemProgressbar;
    }

    @Override
    public void setVisible(boolean visible) {
        if (mOemProgressbar != null) {
            mOemProgressbar.setVisible(visible);
        }
        mVisible = visible;
    }

    @Override
    public boolean isVisible() {
        return mVisible;
    }

    @Override
    public void setIndeterminate(boolean indeterminate) {
        if (mOemProgressbar != null) {
            mOemProgressbar.setIndeterminate(indeterminate);
        }
        mIndeterminate = indeterminate;
    }

    @Override
    public boolean isIndeterminate() {
        return mIndeterminate;
    }

    @Override
    public void setMax(int max) {
        if (mOemProgressbar != null) {
            mOemProgressbar.setMax(max);
        }
        mMax = max;
    }

    @Override
    public int getMax() {
        return mMax;
    }

    @Override
    public void setMin(int min) {
        if (mOemProgressbar != null) {
            mOemProgressbar.setMin(min);
        }
        mMin = min;
    }

    @Override
    public int getMin() {
        return mMin;
    }

    @Override
    public void setProgress(int progress) {
        if (mOemProgressbar != null) {
            mOemProgressbar.setProgress(progress);
        }
        mProgress = progress;
    }

    @Override
    public int getProgress() {
        return mProgress;
    }
}
