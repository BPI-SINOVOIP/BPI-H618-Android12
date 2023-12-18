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

import static com.android.car.ui.utils.CarUiUtils.charSequenceToString;

import android.graphics.drawable.Drawable;

import com.android.car.ui.sharedlibrary.oemapis.toolbar.MenuItemOEMV1;

import java.util.function.Consumer;

/**
 * Adapts a {@link com.android.car.ui.toolbar.MenuItem} into a
 * {@link com.android.car.ui.sharedlibrary.oemapis.toolbar.MenuItemOEMV1}
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class MenuItemAdapterV1 implements MenuItemOEMV1 {

    private final MenuItem mClientMenuItem;
    private Consumer<MenuItemOEMV1> mUpdateListener;

    // This needs to be a member variable because it's only held with a weak listener
    // elsewhere.
    private final MenuItem.Listener mClientListener = menuItem -> {
        if (mUpdateListener != null) {
            mUpdateListener.accept(this);
        }
    };

    public MenuItemAdapterV1(MenuItem item) {
        mClientMenuItem = item;
        item.setListener(mClientListener);
    }

    @Override
    public void setUpdateListener(Consumer<MenuItemOEMV1> listener) {
        mUpdateListener = listener;
    }

    @Override
    public void performClick() {
        mClientMenuItem.performClick();
    }

    @Override
    public int getId() {
        return mClientMenuItem.getId();
    }

    @Override
    public boolean isEnabled() {
        return mClientMenuItem.isEnabled();
    }

    @Override
    public boolean isCheckable() {
        return mClientMenuItem.isCheckable();
    }

    @Override
    public boolean isChecked() {
        return mClientMenuItem.isChecked();
    }

    @Override
    public boolean isTinted() {
        return mClientMenuItem.isTinted();
    }

    @Override
    public boolean isVisible() {
        return mClientMenuItem.isVisible();
    }

    @Override
    public boolean isActivatable() {
        return mClientMenuItem.isActivatable();
    }

    @Override
    public boolean isActivated() {
        return mClientMenuItem.isActivated();
    }

    @Override
    public String getTitle() {
        return charSequenceToString(mClientMenuItem.getTitle());
    }

    @Override
    public boolean isRestricted() {
        return mClientMenuItem.isRestricted();
    }

    @Override
    public boolean isShowingIconAndTitle() {
        return mClientMenuItem.isShowingIconAndTitle();
    }

    @Override
    public boolean isClickable() {
        return mClientMenuItem.getOnClickListener() != null
                || isCheckable()
                || isActivatable();
    }

    @Override
    public int getDisplayBehavior() {
        MenuItem.DisplayBehavior displayBehavior = mClientMenuItem.getDisplayBehavior();
        if (displayBehavior == MenuItem.DisplayBehavior.NEVER) {
            return MenuItemOEMV1.DISPLAY_BEHAVIOR_NEVER;
        } else {
            return MenuItemOEMV1.DISPLAY_BEHAVIOR_ALWAYS;
        }
    }

    @Override
    public Drawable getIcon() {
        return mClientMenuItem.getIcon();
    }

    @Override
    public boolean isPrimary() {
        return mClientMenuItem.isPrimary();
    }

    public boolean isSearch() {
        return mClientMenuItem.isSearch();
    }

    /** Delegates to {@link MenuItem#setVisible(boolean)} */
    public void setVisible(boolean visible) {
        mClientMenuItem.setVisible(visible);
    }
}
