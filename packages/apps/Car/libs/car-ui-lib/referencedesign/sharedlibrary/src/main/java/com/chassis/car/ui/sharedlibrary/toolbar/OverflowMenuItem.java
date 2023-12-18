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
package com.chassis.car.ui.sharedlibrary.toolbar;

import android.app.AlertDialog;
import android.app.Dialog;
import android.content.Context;
import android.graphics.drawable.Drawable;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.MenuItemOEMV1;

import com.chassis.car.ui.sharedlibrary.R;

import java.util.Collections;
import java.util.List;
import java.util.function.Consumer;

class OverflowMenuItem implements MenuItemOEMV1 {

    @NonNull
    private final Context mSharedLibraryContext;

    @NonNull
    private final Context mActivityContext;

    @NonNull
    private List<? extends MenuItemOEMV1> mOverflowMenuItems = Collections.emptyList();

    @Nullable
    private Consumer<MenuItemOEMV1> mUpdateListener;

    @Nullable
    private Dialog mDialog;

    OverflowMenuItem(
            @NonNull Context sharedLibraryContext,
            @NonNull Context activityContext) {
        mSharedLibraryContext = sharedLibraryContext;
        mActivityContext = activityContext;
    }

    public void setOverflowMenuItems(List<? extends MenuItemOEMV1> menuItems) {
        mOverflowMenuItems = menuItems;

        if (mDialog != null) {
            mDialog.dismiss();
            mDialog = null;
        }

        if (mUpdateListener != null) {
            mUpdateListener.accept(this);
        }
    }

    @Override
    public void setUpdateListener(@Nullable Consumer<MenuItemOEMV1> listener) {
        mUpdateListener = listener;
    }

    @Override
    public void performClick() {
        if (!isEnabled() || !isVisible()) {
            return;
        }

        String[] titles = mOverflowMenuItems.stream()
                .map(MenuItemOEMV1::getTitle)
                .toArray(String[]::new);

        mDialog = new AlertDialog.Builder(mActivityContext)
                .setItems(titles, (dialog, which) -> {
                    mOverflowMenuItems.get(which).performClick();
                    dialog.dismiss();
                }).create();
        mDialog.show();
    }

    @Override
    public int getId() {
        return View.NO_ID;
    }

    @Override
    public boolean isEnabled() {
        return true;
    }

    @Override
    public boolean isCheckable() {
        return false;
    }

    @Override
    public boolean isChecked() {
        return false;
    }

    @Override
    public boolean isTinted() {
        return false;
    }

    @Override
    public boolean isVisible() {
        return mOverflowMenuItems.size() > 0;
    }

    @Override
    public boolean isActivatable() {
        return false;
    }

    @Override
    public boolean isActivated() {
        return false;
    }

    @Override
    public String getTitle() {
        return mSharedLibraryContext.getString(R.string.toolbar_menu_item_overflow_title);
    }

    @Override
    public boolean isRestricted() {
        return false;
    }

    @Override
    public boolean isShowingIconAndTitle() {
        return false;
    }

    @Override
    public boolean isClickable() {
        return true;
    }

    @Override
    public int getDisplayBehavior() {
        return MenuItemOEMV1.DISPLAY_BEHAVIOR_ALWAYS;
    }

    @Override
    public Drawable getIcon() {
        return mSharedLibraryContext.getDrawable(R.drawable.toolbar_menu_item_overflow);
    }

    @Override
    public boolean isPrimary() {
        return false;
    }
}
