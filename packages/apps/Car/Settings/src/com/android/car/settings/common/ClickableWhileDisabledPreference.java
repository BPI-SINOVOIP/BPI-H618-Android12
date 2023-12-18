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

import com.android.car.ui.preference.CarUiPreference;

import java.util.function.Consumer;

/**
 * Preference that can be given a disabled look but still be clickable. This enables behavior such
 * as showing a Toast message when clicking on a disabled preference to explain why it's disabled.
 */
public class ClickableWhileDisabledPreference extends CarUiPreference {

    private Consumer<Preference> mClickListener;

    public ClickableWhileDisabledPreference(Context context, AttributeSet attrs,
            int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public ClickableWhileDisabledPreference(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public ClickableWhileDisabledPreference(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public ClickableWhileDisabledPreference(Context context) {
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
