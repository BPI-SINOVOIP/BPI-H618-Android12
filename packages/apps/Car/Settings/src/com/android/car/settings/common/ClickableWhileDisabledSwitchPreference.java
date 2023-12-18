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

import android.content.Context;
import android.util.AttributeSet;

import androidx.annotation.Nullable;
import androidx.preference.Preference;
import androidx.preference.PreferenceViewHolder;

import java.util.function.Consumer;

/**
 * Extends {@link ColoredSwitchPreference} to give a disabled look while it is clickable.
 */
public class ClickableWhileDisabledSwitchPreference extends ColoredSwitchPreference {

    private Consumer<Preference> mClickListener;

    public ClickableWhileDisabledSwitchPreference(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public ClickableWhileDisabledSwitchPreference(Context context, AttributeSet attrs,
            int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public ClickableWhileDisabledSwitchPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ClickableWhileDisabledSwitchPreference(Context context) {
        super(context);
    }

    @Override
    public void onBindViewHolder(PreferenceViewHolder holder) {
        super.onBindViewHolder(holder);
        holder.itemView.setAllowClickWhenDisabled(true);
    }

    @Override
    @SuppressWarnings("RestrictTo")
    public void performClick() {
        if (!isEnabled()) {
            if (mClickListener != null) {
                mClickListener.accept(this);
            }
        } else {
            super.performClick();
        }
    }

    /**
     * Sets the click listener for when the preference is disabled.
     * @param listener Listener to call when the preference is disabled and clicked.
     */
    public void setDisabledClickListener(@Nullable Consumer<Preference> listener) {
        mClickListener = listener;
    }
}
