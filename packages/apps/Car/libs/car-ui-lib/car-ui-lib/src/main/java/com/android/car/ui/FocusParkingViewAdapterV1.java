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

import com.android.car.ui.sharedlibrary.oemapis.FocusParkingViewOEMV1;

/**
 * Adapter from {@link FocusParkingView} to {@link FocusParkingViewOEMV1}.
 */
public class FocusParkingViewAdapterV1 implements FocusParkingViewOEMV1 {
    private final FocusParkingView mFocusParkingView;

    public FocusParkingViewAdapterV1(FocusParkingView focusParkingView) {
        mFocusParkingView = focusParkingView;
    }

    @Override
    public void setShouldRestoreFocus(boolean shouldRestoreFocus) {
        mFocusParkingView.setShouldRestoreFocus(shouldRestoreFocus);
    }

    @Override
    public View getView() {
        return mFocusParkingView;
    }
}
