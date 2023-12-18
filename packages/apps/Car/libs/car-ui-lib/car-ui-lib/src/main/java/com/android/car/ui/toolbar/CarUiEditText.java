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

import android.content.Context;
import android.os.Bundle;
import android.util.AttributeSet;
import android.widget.EditText;

import androidx.annotation.Nullable;

import java.util.function.BiConsumer;

/**
 * Edit text supporting the callbacks from the IMS. This will be useful in widescreen IME mode to
 * allow car-ui-lib to receive responses (like onClick events) from the IMS
 */
@SuppressWarnings("AndroidJdkLibsChecker")
class CarUiEditText extends EditText {

    @Nullable
    private BiConsumer<String, Bundle> mOnAppPrivateCommandListener = null;

    // These need to be public for the layout inflater to inflate them, but
    // checkstyle complains about a public constructor on a package-private class
    //CHECKSTYLE:OFF Generated code
    public CarUiEditText(Context context) {
        super(context);
    }

    public CarUiEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public CarUiEditText(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
    }

    public CarUiEditText(Context context, AttributeSet attrs, int defStyleAttr,
            int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
    }
    //CHECKSTYLE:ON Generated code

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
