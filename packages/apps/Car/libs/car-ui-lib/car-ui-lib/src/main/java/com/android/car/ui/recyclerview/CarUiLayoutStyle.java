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
package com.android.car.ui.recyclerview;

import android.widget.LinearLayout;
import androidx.annotation.IntDef;
import androidx.recyclerview.widget.RecyclerView.LayoutManager;

import com.android.car.ui.recyclerview.CarUiRecyclerView.CarUiRecyclerViewLayout;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * CarUi proxy class for {@link LayoutManager}
 */
public interface CarUiLayoutStyle {

    @IntDef({HORIZONTAL, VERTICAL})
    @Retention(RetentionPolicy.SOURCE)
    @interface Orientation {
    }
    int HORIZONTAL = LinearLayout.HORIZONTAL;
    int VERTICAL = LinearLayout.VERTICAL;

    /** Returns number of recyclerview spans */
    int getSpanCount();

    /** Returns {@link CarUiRecyclerViewLayout} */
    @CarUiRecyclerViewLayout
    int getLayoutType();

    /** Returns layout direction {@link Orientation} */
    @Orientation
    int getOrientation();

    /** Returns true if layout is reversed */
    boolean getReverseLayout();

    /** Returns CarUiRecyclerView size */
    int getSize();
}
