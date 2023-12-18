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

import android.content.Context;
import android.os.Bundle;
import android.util.AttributeSet;
import android.widget.EditText;

import java.util.function.BiConsumer;

/**
 * This is a regular {@link EditText}, but with the addition of a
 * {@link #setOnPrivateImeCommandListener} argument. This allows listening to calls to
 * {@link android.widget.TextView#onPrivateIMECommand(String, Bundle)}.
 */
public class OnPrivateImeCommandEditText extends EditText {

    private BiConsumer<String, Bundle> mOnAppPrivateCommandListener;

    public OnPrivateImeCommandEditText(Context context) {
        super(context);
    }

    public OnPrivateImeCommandEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public OnPrivateImeCommandEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public OnPrivateImeCommandEditText(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }

    @Override
    public boolean onPrivateIMECommand(String action, Bundle data) {
        if (mOnAppPrivateCommandListener != null) {
            mOnAppPrivateCommandListener.accept(action, data);
        }
        return false;
    }

    /**
     * Sets a listener to be called when {@link #onPrivateIMECommand(String, Bundle)} is called.
     */
    public void setOnPrivateImeCommandListener(BiConsumer<String, Bundle> listener) {
        mOnAppPrivateCommandListener = listener;
    }
}
