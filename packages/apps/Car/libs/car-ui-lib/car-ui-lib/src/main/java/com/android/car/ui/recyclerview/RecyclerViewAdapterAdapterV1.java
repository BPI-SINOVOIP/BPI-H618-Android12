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

import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.RecyclerView;
import androidx.recyclerview.widget.RecyclerView.Adapter;
import androidx.recyclerview.widget.RecyclerView.AdapterDataObserver;
import androidx.recyclerview.widget.RecyclerView.ViewHolder;

import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterDataObserverOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.ViewHolderOEMV1;

import java.util.ArrayList;
import java.util.List;

/**
 * AdapterV1 class for delegating calls to AdapterOEMV1
 */
public final class RecyclerViewAdapterAdapterV1
        implements AdapterOEMV1<RecyclerViewAdapterAdapterV1.ViewHolderAdapterV1> {

    @Nullable
    private RecyclerView mRecyclerView;

    @NonNull
    private final Adapter mAdapter;

    @NonNull
    private List<AdapterDataObserverOEMV1> mAdapterDataObservers = new ArrayList<>();

    private AdapterDataObserver mAdapterDataObserver = new AdapterDataObserver() {
        @Override
        public void onChanged() {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                observer.onChanged();
            }
        }

        @Override
        public void onItemRangeChanged(int positionStart, int itemCount) {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                observer.onItemRangeChanged(positionStart, itemCount);
            }
        }

        @Override
        public void onItemRangeChanged(int positionStart, int itemCount,
                @Nullable Object payload) {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                observer.onItemRangeChanged(positionStart, itemCount, payload);
            }
        }

        @Override
        public void onItemRangeInserted(int positionStart, int itemCount) {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                observer.onItemRangeInserted(positionStart, itemCount);
            }
        }

        @Override
        public void onItemRangeRemoved(int positionStart, int itemCount) {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                observer.onItemRangeRemoved(positionStart, itemCount);
            }
        }

        @Override
        public void onItemRangeMoved(int fromPosition, int toPosition, int itemCount) {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                for (int i = 0; i < itemCount; i++) {
                    observer.onItemMoved(fromPosition + i, toPosition + i);
                }
            }
        }

        @Override
        public void onStateRestorationPolicyChanged() {
            for (AdapterDataObserverOEMV1 observer: mAdapterDataObservers) {
                observer.onStateRestorationPolicyChanged();
            }
        }
    };

    public RecyclerViewAdapterAdapterV1(@NonNull RecyclerView.Adapter adapter) {
        mAdapter = adapter;
    }

    @NonNull
    public RecyclerView.Adapter getAdapter() {
        return mAdapter;
    }

    @Override
    public int getItemCount() {
        return mAdapter.getItemCount();
    }

    @Override
    public long getItemId(int position) {
        return mAdapter.getItemId(position);
    }

    @Override
    public int getItemViewType(int position) {
        return mAdapter.getItemViewType(position);
    }

    @Override
    public int getStateRestorationPolicyInt() {
        switch (mAdapter.getStateRestorationPolicy()) {
            case PREVENT:
                return AdapterOEMV1.PREVENT;
            case PREVENT_WHEN_EMPTY:
                return AdapterOEMV1.PREVENT_WHEN_EMPTY;
            case ALLOW:
            default:
                return AdapterOEMV1.ALLOW;
        }
    }

    @Override
    public void onAttachedToRecyclerView(RecyclerViewOEMV1 recyclerView) {
        if (mRecyclerView != null) {
            mAdapter.onAttachedToRecyclerView(mRecyclerView);
            mAdapter.registerAdapterDataObserver(mAdapterDataObserver);
        }
    }

    @Override
    public void bindViewHolder(ViewHolderAdapterV1 holder, int position) {
        mAdapter.bindViewHolder(holder.getViewHolder(), position);
    }

    @Override
    public ViewHolderAdapterV1 createViewHolder(ViewGroup parent, int viewType) {
        return new ViewHolderAdapterV1(mAdapter.createViewHolder(parent, viewType));
    }

    @Override
    public void onDetachedFromRecyclerView(RecyclerViewOEMV1 recyclerView) {
        if (mRecyclerView != null) {
            mAdapter.unregisterAdapterDataObserver(mAdapterDataObserver);
            mAdapter.onDetachedFromRecyclerView(mRecyclerView);
        }
    }

    @Override
    public boolean onFailedToRecycleView(ViewHolderAdapterV1 holder) {
        return mAdapter.onFailedToRecycleView(holder.getViewHolder());
    }

    @Override
    public void onViewAttachedToWindow(ViewHolderAdapterV1 holder) {
        mAdapter.onViewAttachedToWindow(holder.getViewHolder());
    }

    @Override
    public void onViewDetachedFromWindow(ViewHolderAdapterV1 holder) {
        mAdapter.onViewDetachedFromWindow(holder.getViewHolder());
    }

    @Override
    public void onViewRecycled(ViewHolderAdapterV1 holder) {
        mAdapter.onViewRecycled(holder.getViewHolder());
    }

    @Override
    public void registerAdapterDataObserver(AdapterDataObserverOEMV1 observer) {
        if (observer == null) {
            return;
        }
        mAdapterDataObservers.add(observer);
    }

    @Override
    public void unregisterAdapterDataObserver(AdapterDataObserverOEMV1 observer) {
        if (observer == null) {
            return;
        }
        mAdapterDataObservers.remove(observer);
    }

    @Override
    public boolean hasStableIds() {
        return mAdapter.hasStableIds();
    }

    @Override
    public void setRecyclerView(@Nullable View recyclerview) {
        mRecyclerView = (RecyclerView) recyclerview;
    }

    static class ViewHolderAdapterV1 implements ViewHolderOEMV1 {

        private RecyclerView.ViewHolder mViewHolder;

        ViewHolderAdapterV1(@NonNull RecyclerView.ViewHolder viewHolder) {
            mViewHolder = viewHolder;
        }

        @NonNull
        public ViewHolder getViewHolder() {
            return mViewHolder;
        }

        @Override
        public boolean isRecyclable() {
            return mViewHolder.isRecyclable();
        }

        @Override
        public View getItemView() {
            return mViewHolder.itemView;
        }
    }
}
