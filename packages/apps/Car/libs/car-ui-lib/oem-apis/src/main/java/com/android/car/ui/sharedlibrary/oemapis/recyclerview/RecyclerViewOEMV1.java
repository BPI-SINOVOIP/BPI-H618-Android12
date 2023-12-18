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
package com.android.car.ui.sharedlibrary.oemapis.recyclerview;

import android.view.View;

/**
 * {@link androidx.recyclerview.widget.RecyclerView}
 */
public interface RecyclerViewOEMV1 {

    /** {@link androidx.recyclerview.widget.RecyclerView#setAdapter(Adapter)} */
    void setAdapter(AdapterOEMV1 adapter);

    /** {@link androidx.recyclerview.widget.RecyclerView#addOnScrollListener} */
    void addOnScrollListener(OnScrollListenerOEMV1 listener);

    /** {@link androidx.recyclerview.widget.RecyclerView#removeOnScrollListener} */
    void removeOnScrollListener(OnScrollListenerOEMV1 listener);

    /** {@link androidx.recyclerview.widget.RecyclerView#clearOnScrollListeners()} */
    void clearOnScrollListeners();

    /** {@link androidx.recyclerview.widget.RecyclerView#scrollToPosition(int)} */
    void scrollToPosition(int position);

    /** {@link androidx.recyclerview.widget.RecyclerView#smoothScrollBy(int, int)} */
    void smoothScrollBy(int dx, int dy);

    /** {@link androidx.recyclerview.widget.RecyclerView#smoothScrollToPosition(int)} */
    void smoothScrollToPosition(int position);

    /** {@link androidx.recyclerview.widget.RecyclerView#setHasFixedSize(boolean)} */
    void setHasFixedSize(boolean hasFixedSize);

    /** {@link androidx.recyclerview.widget.RecyclerView#hasFixedSize()} */
    boolean hasFixedSize();

    /**
     * set {@link LayoutStyleOEMV1}. This is the replacement for
     * {@link androidx.recyclerview.widget.RecyclerView.LayoutManager}
     */
    void setLayoutStyle(LayoutStyleOEMV1 layoutStyle);

    /**
     * Returns the view that will be displayed on the screen.
     */
    View getView();

    /** {@link android.view.View#setPadding(int, int, int, int)} */
    void setPadding(int left, int top, int right, int bottom);

    /** {@link android.view.View#setPaddingRelative(int, int, int, int)} */
    void setPaddingRelative(int start, int top, int end, int bottom);

    /** {@link androidx.recyclerview.widget.RecyclerView#setClipToPadding(boolean)} */
    void setClipToPadding(boolean clipToPadding);

    /**
     * Return's the container which contains the scrollbar and this RecyclerView.
     */
    View getContainer();
}
