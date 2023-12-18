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
package com.android.car.ui.sharedlibrary.oemapis.toolbar;

import android.os.Bundle;
import android.widget.TextView;

import java.util.function.BiConsumer;
import java.util.function.Consumer;

/**
 * This is an interface (as in "bridge") between the static lib and the shared lib, so that
 * the static lib can handle showing search results inside the IME.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public interface ImeSearchInterfaceOEMV1 {
    /**
     * Sets a consumer that should be called when the search TextView is ready.
     * <p>
     * This is for the widescreen IME functionality, which is implemented in the static lib. If
     * this implementation of a Toolbar does not need widescreen IME functionality,
     * this method may return null.
     *
     * @see #setOnPrivateImeCommandListener
     */
    void setSearchTextViewConsumer(Consumer<TextView> textViewConsumer);

    /**
     * Sets a listener to be called when the TextView supplied to the consumer set in
     * {@link #setSearchTextViewConsumer} receives a
     * {@link TextView#onPrivateIMECommand(String, Bundle)} call.
     * <p>
     * This is for the widescreen IME functionality, which is implemented in the static lib. If
     * this implementation of a Toolbar does not need widescreen IME functionality,
     * this method may do nothing.
     *
     * @see #setSearchTextViewConsumer
     */
    void setOnPrivateImeCommandListener(BiConsumer<String, Bundle> onPrivateImeCommandListener);
}
