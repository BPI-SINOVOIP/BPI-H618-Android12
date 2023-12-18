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

import android.app.Dialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import androidx.annotation.NonNull;

import com.android.car.ui.appstyledview.AppStyledViewController.AppStyledDismissListener;

/**
 * App styled dialog used to display a view that cannot be customized via OEM. Dialog will inflate a
 * layout and add the view provided by the application into the layout. Everything other than the
 * view within the layout can be customized by OEM.
 *
 * Apps should not use this directly. App's should use {@link AppStyledDialogController}.
 */
public class AppStyledDialog extends Dialog implements DialogInterface.OnDismissListener {

    private final AppStyledViewController mController;
    private AppStyledDismissListener mOnDismissListener;
    private View mContent;

    public AppStyledDialog(@NonNull Context context, AppStyledViewController controller) {
        super(context);
        mController = controller;
        setOnDismissListener(this);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        requestWindowFeature(Window.FEATURE_NO_TITLE);
        setContentView(mController.getAppStyledView(mContent));

        WindowManager.LayoutParams params = getWindow().getAttributes();
        getWindow().setAttributes(mController.getDialogWindowLayoutParam(params));
    }

    @Override
    public void onDismiss(DialogInterface dialog) {
        if (mOnDismissListener != null) {
            mOnDismissListener.onDismiss();
        }
    }

    void setContent(View contentView) {
        mContent = contentView;
    }

    void setOnDismissListener(AppStyledDismissListener listener) {
        mOnDismissListener = listener;
    }

    WindowManager.LayoutParams getWindowLayoutParams() {
        return getWindow().getAttributes();
    }
}
