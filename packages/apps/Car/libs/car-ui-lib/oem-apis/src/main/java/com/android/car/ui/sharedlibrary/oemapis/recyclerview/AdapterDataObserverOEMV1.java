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

/**
 * {@link androidx.recyclerview.widget.RecyclerView.Adapter#registerAdapterDataObserver}
 */
public interface AdapterDataObserverOEMV1 {

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onChanged()} */
    void onChanged();

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onItemRangeChanged(int, int)} */
    void onItemRangeChanged(int positionStart, int itemCount);

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onItemRangeChanged(int, int, Object)} */
    void onItemRangeChanged(int positionStart, int itemCount, Object payload);

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onItemRangeInserted(int, int)} */
    void onItemRangeInserted(int positionStart, int itemCount);

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onItemRangeRemoved(int, int)} */
    void onItemRangeRemoved(int positionStart, int itemCount);

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onItemRangeMoved(int, int, int)} */
    void onItemMoved(int fromPosition, int toPosition);

    /** {@link androidx.recyclerview.widget.RecyclerView.AdapterDataObserver#onStateRestorationPolicyChanged()} */
    void onStateRestorationPolicyChanged();
}
