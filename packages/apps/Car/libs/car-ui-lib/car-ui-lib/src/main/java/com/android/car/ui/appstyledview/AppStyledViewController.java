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

import static java.lang.annotation.RetentionPolicy.SOURCE;

import android.view.View;
import android.view.WindowManager;

import androidx.annotation.IntDef;

import java.lang.annotation.Retention;

/**
 * An interface for accessing a Chassis AppStyledView, regardless of how the underlying views are
 * represented.
 */
public interface AppStyledViewController {

    /**
     * Callback to be invoked when the close icon is clicked.
     */
    interface AppStyledVCloseClickListener {

        /**
         * Called when the close icon is clicked.
         */
        void onClick();
    }

    /**
     * Callback to be invoked when the dialog is dismissed.
     */
    interface AppStyledDismissListener {

        /**
         * Called when the dialog is dismissed.
         */
        void onDismiss();
    }

    /**
     * The possible values for AppStyledViewNavIcon.
     */
    @IntDef({
            AppStyledViewNavIcon.BACK,
            AppStyledViewNavIcon.CLOSE,
    })
    @Retention(SOURCE)
    @interface AppStyledViewNavIcon {

        /**
         * Show a back icon
         */
        int BACK = 0;

        /**
         * Show a close icon
         */
        int CLOSE = 1;
    }

    /**
     * Creates a app styled view.
     *
     * @return the view used for app styled view.
     */
    View getAppStyledView(View contentView);

    /**
     * Sets the nav icon to be used.
     */
    void setNavIcon(@AppStyledViewNavIcon int navIcon);

    /**
     * Sets the {@link AppStyledVCloseClickListener}
     */
    void setOnCloseClickListener(AppStyledVCloseClickListener listener);

    /**
     * Returns the layout params for the AppStyledView dialog
     */
    WindowManager.LayoutParams getDialogWindowLayoutParam(WindowManager.LayoutParams params);
}
