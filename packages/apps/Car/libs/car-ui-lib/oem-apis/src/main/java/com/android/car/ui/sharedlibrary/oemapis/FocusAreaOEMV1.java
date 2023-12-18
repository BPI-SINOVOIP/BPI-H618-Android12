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
import android.widget.LinearLayout;

/**
 * The OEM interface for a FocusArea. Unlike most components, the FocusArea has it's implementation
 * in the static library, and this interface is to give the shared library access to it. The
 * shared library is not expected to implement this interface.
 * <p>
 * See {@link SharedLibraryFactoryOEMV1#setRotaryFactories}
 */
public interface FocusAreaOEMV1 {

    /**
     * Sets the padding (in pixels) of the FocusArea highlight.
     * <p>
     * It doesn't affect other values, such as the paddings on its child views.
     */
    void setHighlightPadding(int left, int top, int right, int bottom);

    /**
     * Sets the offset (in pixels) of the FocusArea's bounds.
     * <p>
     * It only affects the perceived bounds for the purposes of finding the nudge target. It doesn't
     * affect the FocusArea's view bounds or highlight bounds. The offset should only be used when
     * FocusAreas are overlapping and nudge interaction is ambiguous.
     */
    void setBoundsOffset(int left, int top, int right, int bottom);

    /**
     * Sets whether wrap-around is enabled for this FocusArea. Wrap-around being enabled means
     * that when you try to move focus to the right from the rightmost view, the focus will
     * wrap around to the leftmost view.
     */
    void setWrapAround(boolean wrapAround);

    /** Sets the default focus view in this FocusArea. */
    void setDefaultFocus(View defaultFocus);

    /**
     * Sets the nudge shortcut for the given {@code direction}. Removes the nudge shortcut if {@code
     * view} is {@code null}.
     */
    void setNudgeShortcut(int direction, View view);

    /**
     * Gets the underlying view that this object represents. This is just for adding the view
     * to the rest of the view hierarchy; no methods that change the appearance/behavior of the
     * view should be called on it.
     */
    LinearLayout getView();
}
