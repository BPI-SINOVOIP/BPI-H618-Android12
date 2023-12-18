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
package com.android.car.ui.appstyledview;

import android.view.View;
import android.view.WindowManager.LayoutParams;

import androidx.annotation.NonNull;

import com.android.car.ui.sharedlibrary.oemapis.appstyledview.AppStyledViewControllerOEMV1;

/**
 * Adapts a {@link AppStyledViewControllerOEMV1} into a {@link AppStyledViewController}
 */
public class AppStyledViewControllerAdapterV1 implements AppStyledViewController {

    @NonNull
    private final AppStyledViewControllerOEMV1 mOemController;

    public AppStyledViewControllerAdapterV1(AppStyledViewControllerOEMV1 controllerOEMV1) {
        mOemController = controllerOEMV1;
    }

    /**
     * Returns the view that will be displayed on the screen.
     */
    @Override
    public View getAppStyledView(View contentView) {
        return mOemController.getAppStyledView(contentView);
    }

    @Override
    public void setNavIcon(int navIcon) {
        mOemController.setNavIcon(navIcon);
    }

    @Override
    public void setOnCloseClickListener(AppStyledVCloseClickListener listener) {
        mOemController.setOnCloseClickListener(listener::onClick);
    }

    @Override
    public LayoutParams getDialogWindowLayoutParam(LayoutParams params) {
        return mOemController.getDialogWindowLayoutParam(params);
    }
}
