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

package com.android.car.ui.sharedlibrary.oemapis.toolbar;

import android.graphics.drawable.Drawable;

import java.util.function.Consumer;

/** The OEM interface of a MenuItem, which is a button in the toolbar */
@SuppressWarnings("AndroidJdkLibsChecker")
public interface MenuItemOEMV1 {

    /** Sets a listener that will be called when any property of the MenuItem changes */
    void setUpdateListener(Consumer<MenuItemOEMV1> listener);

    /**
     * Triggers the MenuItem being clicked. This will toggle it's activated/checked state
     * if it supports those, and also call it's onClickListener.
     */
    void performClick();

    /** Gets the id, which is purely for the client to distinguish MenuItems with. */
    int getId();

    /** Returns whether the MenuItem is enabled */
    boolean isEnabled();

    /** Returns whether the MenuItem is checkable. If it is, it will be displayed as a switch. */
    boolean isCheckable();

    /**
     * Returns whether the MenuItem is currently checked. Only valid if {@link #isCheckable()}
     * is true.
     */
    boolean isChecked();

    /** Whether or not to tint the Icon to match the theme of the toolbar */
    boolean isTinted();

    /** Returns whether or not the MenuItem is visible */
    boolean isVisible();

    /**
     * Returns whether the MenuItem is activatable. If it is, it's every click will toggle
     * the MenuItem's View to appear activated or not.
     */
    boolean isActivatable();

    /** Returns whether or not this view is selected. Toggles after every click */
    boolean isActivated();

    /** Gets the title of this MenuItem. */
    String getTitle();

    /**
     * Returns if this MenuItem is restricted due to the current driving restrictions and driving
     * state. It should be displayed visually distinctly to indicate that.
     */
    boolean isRestricted();

    /**
     * Returns if both the icon and title should be shown. If not, and they're both provided,
     * only the icon will be shown and the title will be used as a content description.
     */
    boolean isShowingIconAndTitle();

    /**
     * Returns if the MenuItem should do something when clicked. This can be used to forgo
     * setting an onClickListener on it's View when it's not clickable.
     */
    boolean isClickable();

    /** Always show the MenuItem on the toolbar */
    int DISPLAY_BEHAVIOR_ALWAYS = 0;
    /** Show the MenuItem in the toolbar if there's space, otherwise show it in the overflow menu */
    int DISPLAY_BEHAVIOR_IF_ROOM = 1;
    /** Never show the MenuItem on the toolbar, always put it in an overflow menu */
    int DISPLAY_BEHAVIOR_NEVER = 2;

    /**
     * Gets the current display behavior.
     *
     * See {@link #DISPLAY_BEHAVIOR_ALWAYS}, {@link #DISPLAY_BEHAVIOR_IF_ROOM}, and
     * {@link #DISPLAY_BEHAVIOR_NEVER}.
     */
    int getDisplayBehavior();

    /** Gets the current Icon */
    Drawable getIcon();

    /**
     * Returns if this MenuItem is a primary one, which should be visually different.
     *
     * This value will not change, even after an update was triggered.
     */
    boolean isPrimary();
}
