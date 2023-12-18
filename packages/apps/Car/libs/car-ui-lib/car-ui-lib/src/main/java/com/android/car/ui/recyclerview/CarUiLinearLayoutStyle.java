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

import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView.LayoutManager;

import com.android.car.ui.recyclerview.CarUiRecyclerView.CarUiRecyclerViewLayout;

/**
 * CarUi proxy class for {@link LinearLayoutManager}
 */
public final class CarUiLinearLayoutStyle implements CarUiLayoutStyle {

    @CarUiRecyclerViewLayout
    private  int mLayoutType = CarUiRecyclerViewLayout.LINEAR;
    @Orientation
    private int mLayoutOrientation = VERTICAL;
    private boolean mReverseLayout = false;
    private int mSize = CarUiRecyclerView.SIZE_LARGE;

    /**
     * @param layoutManager
     * @return instance {@link CarUiLayoutStyle} using the passed {@link LayoutManager}
     */
    @Nullable
    public static CarUiLinearLayoutStyle from(@Nullable LayoutManager layoutManager) {
        if (layoutManager == null) return null;
        if (!(layoutManager instanceof LinearLayoutManager)) {
            throw new AssertionError("LinearLayoutManager required.");
        }

        CarUiLinearLayoutStyle layoutStyle = new CarUiLinearLayoutStyle();
        layoutStyle.setOrientation(((LinearLayoutManager) layoutManager).getOrientation());
        layoutStyle.setReverseLayout(((LinearLayoutManager) layoutManager).getReverseLayout());
        return layoutStyle;
    }

    /** Returns number of recyclerview spans */
    public int getSpanCount() {
        return 1;
    }

    /** Returns {@link CarUiRecyclerViewLayout} */
    @CarUiRecyclerViewLayout
    public int getLayoutType() {
        return CarUiRecyclerViewLayout.LINEAR;
    }

    /** Returns layout direction {@link Orientation} */
    @Orientation
    public int getOrientation() {
        return mLayoutOrientation;
    }

    /** sets layout direction {@link Orientation} */
    public void setOrientation(@Orientation int orientation) {
        mLayoutOrientation = orientation;
    }

    /** Returns true if layout is reversed */
    public boolean getReverseLayout() {
        return mReverseLayout;
    }

    /** sets if layout is reversed */
    public void setReverseLayout(boolean reverseLayout) {
        mReverseLayout = reverseLayout;
    }

    /**
     * @return CarUiRecyclerView size
     */
    public int getSize() {
        return mSize;
    }

    /**
     * @param size CarUiRecyclerView size
     */
    public void setSize(int size) {
        mSize = size;
    }
}
