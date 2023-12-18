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

package com.android.car.settings.common;

import android.widget.FrameLayout;

import androidx.preference.Preference;

import java.util.function.Consumer;

/**
 * Class representing an action item for an {@link MultiActionPreference}
 */
public abstract class BaseActionItem {
    protected Preference mPreference;
    protected boolean mIsPreferenceRestricted = false;
    protected Consumer<Preference> mRestrictedOnClickListener;

    private boolean mIsEnabled = true;
    private boolean mIsVisible = true;
    private ActionItemInfoChangeListener mActionItemInfoChangeListener;

    public BaseActionItem(ActionItemInfoChangeListener actionItemInfoChangeListener) {
        mActionItemInfoChangeListener = actionItemInfoChangeListener;
    }

    /**
     * Get the enabled state.
     */
    public boolean isEnabled() {
        return mIsEnabled;
    }

    /**
     * Set the enabled state.
     */
    public void setEnabled(boolean enabled) {
        if (enabled != mIsEnabled) {
            mIsEnabled = enabled;
            update();
        }
    }

    /**
     * Get the visibility state.
     */
    public boolean isVisible() {
        return mIsVisible;
    }

    /**
     * Set the visibility state.
     */
    public void setVisible(boolean visible) {
        if (visible != mIsVisible) {
            mIsVisible = visible;
            update();
        }
    }

    BaseActionItem setPreference(Preference preference) {
        mPreference = preference;
        return this;
    }

    BaseActionItem setPreferenceRestricted(boolean isPreferenceRestricted) {
        mIsPreferenceRestricted = isPreferenceRestricted;
        return this;
    }

    BaseActionItem setRestrictedOnClickListener(Consumer<Preference> restrictedOnClickListener) {
        mRestrictedOnClickListener = restrictedOnClickListener;
        return this;
    }

    /**
     * Notify listener that action item has been changed.
     */
    protected void update() {
        ActionItemInfoChangeListener listener = mActionItemInfoChangeListener;
        if (listener != null) {
            listener.onActionItemChange(this);
        }
    }

    abstract void bindViewHolder(FrameLayout frameLayout);
    abstract int getLayoutResource();

    /**
     * Listener that is notified when an action item has been changed.
     */
    interface ActionItemInfoChangeListener {
        void onActionItemChange(BaseActionItem baseActionItem);
    }
}
