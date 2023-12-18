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
package com.android.car.ui.sharedlibrary.oemapis;

import android.view.View;

/**
 * The OEM interface for a FocusParkingView. Unlike most components, the FocusParkingView has it's
 * implementation in the static library, and this interface is to give the shared library access to
 * it. The shared library is not expected to implement this interface.
 * <p>
 * See {@link SharedLibraryFactoryOEMV1#setRotaryFactories}
 */
public interface FocusParkingViewOEMV1 {

    /**
     * Sets whether this view should restore focus when the framework wants to focus this view. When
     * set to false, this view allows itself to be focused instead. This should be set to false for
     * the {@code FocusParkingView} in a {@code TaskView}. The default value is true.
     */
    void setShouldRestoreFocus(boolean shouldRestoreFocus);

    /**
     * Gets the underlying view that this object represents. This is just for adding the view
     * to the rest of the view hierarchy; no methods that change the appearance/behavior of the
     * view should be called on it.
     * <p>
     * This should be the first focusable view in the view hierarchy, but other than that
     * the position/size doesn't matter. It's recommended to use wrap_content as both the width
     * and height in the LayoutParams.
     */
    View getView();
}
