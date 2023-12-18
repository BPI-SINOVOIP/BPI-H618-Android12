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
package com.android.car.ui;

import android.view.View;
import android.widget.LinearLayout;

import com.android.car.ui.sharedlibrary.oemapis.FocusAreaOEMV1;

/**
 * Adapter from {@link FocusArea} to {@link FocusAreaOEMV1}.
 */
public class FocusAreaAdapterV1 implements FocusAreaOEMV1 {
    private final FocusArea mFocusArea;

    public FocusAreaAdapterV1(FocusArea focusArea) {
        mFocusArea = focusArea;
    }

    @Override
    public void setHighlightPadding(int left, int top, int right, int bottom) {
        mFocusArea.setHighlightPadding(left, top, right, bottom);
    }

    @Override
    public void setBoundsOffset(int left, int top, int right, int bottom) {
        mFocusArea.setBoundsOffset(left, top, right, bottom);
    }

    @Override
    public void setWrapAround(boolean wrapAround) {
        mFocusArea.setWrapAround(wrapAround);
    }

    @Override
    public void setDefaultFocus(View defaultFocus) {
        mFocusArea.setDefaultFocus(defaultFocus);
    }

    @Override
    public void setNudgeShortcut(int direction, View view) {
        mFocusArea.setNudgeShortcut(direction, view);
    }

    @Override
    public LinearLayout getView() {
        return mFocusArea;
    }
}
