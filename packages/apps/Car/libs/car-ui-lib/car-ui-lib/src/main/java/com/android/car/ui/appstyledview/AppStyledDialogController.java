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

import android.content.Context;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.VisibleForTesting;

import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledDismissListener;
import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledVCloseClickListener;
import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledViewNavIcon;
import com.android.car.ui.sharedlibrarysupport.SharedLibraryFactorySingleton;

import java.util.Objects;

/**
 * Controller to interact with the app styled view UI.
 */
public final class AppStyledDialogController {

    @NonNull
    private AppStyledViewController mAppStyledViewController;
    @NonNull
    private AppStyledDialog mDialog;

    public AppStyledDialogController(@NonNull Context context) {
        Objects.requireNonNull(context);
        mAppStyledViewController = SharedLibraryFactorySingleton.get(context)
                .createAppStyledView(context);
        mDialog = new AppStyledDialog(context, mAppStyledViewController);
    }

    @VisibleForTesting
    void setAppStyledViewController(AppStyledViewController controller, Context context) {
        mAppStyledViewController = controller;
        mDialog = new AppStyledDialog(context, mAppStyledViewController);
    }

    /**
     * Sets the content view to de displayed in AppStyledView.
     */
    public void setContentView(@NonNull View contentView) {
        Objects.requireNonNull(contentView);

        mDialog.setContent(contentView);
        mAppStyledViewController.setOnCloseClickListener(mDialog::dismiss);
    }

    /**
     * Sets the nav icon to be used.
     */
    public void setNavIcon(@AppStyledViewNavIcon int navIcon) {
        mAppStyledViewController.setNavIcon(navIcon);
    }

    /**
     * Displays the dialog to the user with the custom view provided by the app.
     */
    public void show() {
        mDialog.show();
    }

    /**
     * Sets the {@link AppStyledVCloseClickListener}
     */
    public void setOnCloseClickListener(@NonNull AppStyledVCloseClickListener listener) {
        mAppStyledViewController.setOnCloseClickListener(() -> {
            mDialog.dismiss();
            listener.onClick();
        });
    }

    /**
     * Sets the {@link AppStyledDismissListener}
     */
    public void setOnDismissListener(@NonNull AppStyledDismissListener listener) {
        mDialog.setOnDismissListener(listener);
    }

    /**
     * Returns the width of the AppStyledView
     */
    public int getAppStyledViewDialogWidth() {
        return mAppStyledViewController.getDialogWindowLayoutParam(
                mDialog.getWindowLayoutParams()).width;
    }

    /**
     * Returns the height of the AppStyledView
     */
    public int getAppStyledViewDialogHeight() {
        return mAppStyledViewController.getDialogWindowLayoutParam(
                mDialog.getWindowLayoutParams()).height;
    }

    @VisibleForTesting
    AppStyledDialog getAppStyledDialog() {
        return mDialog;
    }
}
