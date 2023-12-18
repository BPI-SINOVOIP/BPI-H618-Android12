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

package com.android.car.rotaryplayground;

import android.content.Context;
import android.util.AttributeSet;
import android.widget.Button;

import androidx.annotation.Nullable;

public class CustomButton extends Button {

    private static final int[] STATE_ROTARY_ENABLED = {R.attr.state_rotary_enabled};

    private boolean mRotaryEnabled = true;

    public CustomButton(Context context) {
        super(context);
    }

    public CustomButton(Context context, @Nullable AttributeSet attrs) {
        super(context, attrs);
    }

    public CustomButton(Context context, @Nullable AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public CustomButton(Context context, @Nullable AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    public void setRotaryEnabled(boolean enabled) {
        mRotaryEnabled = enabled;
        refreshDrawableState();
    }

    public boolean isRotaryEnabled() {
        return mRotaryEnabled;
    }

    @Override
    protected int[] onCreateDrawableState(int extraSpace) {
        final int[] drawableState = super.onCreateDrawableState(extraSpace + 1);
        if (mRotaryEnabled) {
            mergeDrawableStates(drawableState, STATE_ROTARY_ENABLED);
        }
        return drawableState;
    }
}
