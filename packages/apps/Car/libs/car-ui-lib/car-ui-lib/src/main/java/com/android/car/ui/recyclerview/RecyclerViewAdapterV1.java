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

import android.content.Context;
import android.util.AttributeSet;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.R;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.LayoutStyleOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.OnScrollListenerOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.SpanSizeLookupOEMV1;

import java.util.ArrayList;
import java.util.List;

/**
 * AdapterV1 class for making oem implementation available for UI
 *
 * For CarUi internal usage only.
 */
public final class RecyclerViewAdapterV1 extends CarUiRecyclerView
        implements OnScrollListenerOEMV1 {

    @Nullable
    private RecyclerViewOEMV1 mOEMRecyclerView;
    @Nullable
    private AdapterOEMV1 mOEMAdapter;

    private List<OnScrollListener> mScrollListeners = new ArrayList<>();

    public RecyclerViewAdapterV1(@NonNull Context context) {
        this(context, null);
    }

    public RecyclerViewAdapterV1(@NonNull Context context, @Nullable AttributeSet attrs) {
        this(context, attrs, R.attr.carUiRecyclerViewStyle);
    }

    public RecyclerViewAdapterV1(@NonNull Context context, @Nullable AttributeSet attrs,
            int defStyle) {
        super(context, attrs, defStyle);
    }

    /**
     * Called to pass the oem recyclerview implementation.
     * @param oemRecyclerView
     */
    public void setRecyclerViewOEMV1(@NonNull RecyclerViewOEMV1 oemRecyclerView) {
        mOEMRecyclerView = oemRecyclerView;

        mOEMRecyclerView.addOnScrollListener(this);
        super.setLayoutManager(new LinearLayoutManager(getContext()));
        View oemRV = oemRecyclerView.getView();
        ViewGroup.LayoutParams params = new MarginLayoutParams(ViewGroup.LayoutParams.MATCH_PARENT,
                ViewGroup.LayoutParams.WRAP_CONTENT);
        oemRV.setLayoutParams(params);
        RecyclerView.Adapter adapter = new CustomAdapter(oemRV);
        super.setAdapter(adapter);
    }

    @Override
    public void setLayoutManager(@Nullable LayoutManager layoutManager) {
        if (layoutManager instanceof LinearLayoutManager) {
            setLayoutStyle(CarUiLinearLayoutStyle.from(layoutManager));
        } else {
            setLayoutStyle(CarUiGridLayoutStyle.from(layoutManager));
        }
    }

    @Override
    public View getContainer() {
        return mOEMRecyclerView.getContainer();
    }

    @Override
    public void setAdapter(RecyclerView.Adapter adapter) {
        if (mOEMAdapter != null) {
            mOEMAdapter.setRecyclerView(null);
        }

        if (adapter == null) {
            mOEMRecyclerView.setAdapter(null);
        } else {
            mOEMAdapter = new RecyclerViewAdapterAdapterV1(adapter);
            mOEMRecyclerView.setAdapter(mOEMAdapter);
            mOEMAdapter.setRecyclerView(this);
        }
    }

    @Override
    public void addOnScrollListener(@NonNull RecyclerView.OnScrollListener listener) {
        mScrollListeners.add(listener);
    }

    @Override
    public void removeOnScrollListener(@NonNull RecyclerView.OnScrollListener listener) {
        if (mScrollListeners != null) {
            mScrollListeners.remove(listener);
        }
    }

    @Override
    public void clearOnScrollListeners() {
        if (mScrollListeners != null) {
            mScrollListeners.clear();
        }
        mOEMRecyclerView.clearOnScrollListeners();
    }

    @Override
    public void onScrollStateChanged(@NonNull RecyclerViewOEMV1 recyclerView, int newState) {
        if (mScrollListeners != null) {
            for (RecyclerView.OnScrollListener listener: mScrollListeners) {
                listener.onScrollStateChanged(this, newState);
            }
        }
    }

    @Override
    public void onScrolled(@NonNull RecyclerViewOEMV1 recyclerView, int dx, int dy) {
        if (mScrollListeners != null) {
            for (RecyclerView.OnScrollListener listener: mScrollListeners) {
                listener.onScrolled(this, dx, dy);
            }
        }
    }

    @Override
    public void scrollToPosition(int position) {
        mOEMRecyclerView.scrollToPosition(position);
    }

    @Override
    public void smoothScrollBy(int dx, int dy) {
        mOEMRecyclerView.smoothScrollBy(dx, dy);
    }

    @Override
    public void smoothScrollToPosition(int position) {
        mOEMRecyclerView.smoothScrollToPosition(position);
    }

    @Override
    public ViewHolder findViewHolderForAdapterPosition(int position) {
        // TODO
        return null;
    }

    @Override
    public void setHasFixedSize(boolean hasFixedSize) {
        mOEMRecyclerView.setHasFixedSize(hasFixedSize);
    }

    @Override
    public boolean hasFixedSize() {
        return mOEMRecyclerView.hasFixedSize();
    }

    private static class CustomAdapter extends RecyclerView.Adapter<RecyclerView.ViewHolder> {

        @NonNull
        private View mView;

        CustomAdapter(@NonNull View view) {
            mView = view;
        }

        @Override
        public int getItemCount() {
            return 1;
        }

        @NonNull
        @Override
        public RecyclerView.ViewHolder onCreateViewHolder(
                @NonNull ViewGroup parent, int viewType) {
            return new CustomViewHolder(mView);
        }

        @Override
        public void onBindViewHolder(@NonNull RecyclerView.ViewHolder holder, int position) {
        }

        static class CustomViewHolder extends RecyclerView.ViewHolder {
            CustomViewHolder(@NonNull View itemView) {
                super(itemView);
            }
        }
    }

    /**
     * @deprecated LayoutManager will be implemented by OEMs,
     * use other available APIs to get the required data
     * @return null
     */
    @Nullable
    @Override
    @Deprecated
    public LayoutManager getLayoutManager() {
        return null;
    }

    @Override
    public void setLayoutStyle(@Nullable CarUiLayoutStyle layoutStyle) {
        if (layoutStyle == null) mOEMRecyclerView.setLayoutStyle(null);

        final LayoutStyleOEMV1 oemLayoutStyle = new LayoutStyleOEMV1() {
            @Override
            public int getSpanCount() {
                return layoutStyle.getSpanCount();
            }

            @Override
            public int getLayoutType() {
                return layoutStyle.getLayoutType();
            }

            @Override
            public int getOrientation() {
                return layoutStyle.getOrientation();
            }

            @Override
            public boolean getReverseLayout() {
                return layoutStyle.getReverseLayout();
            }

            @Override
            public SpanSizeLookupOEMV1 getSpanSizeLookup() {
                if (layoutStyle instanceof CarUiLinearLayoutStyle) return null;
                return ((CarUiGridLayoutStyle) layoutStyle).getSpanSizeLookup() == null ? null
                        : position -> ((CarUiGridLayoutStyle) layoutStyle)
                                .getSpanSizeLookup().getSpanSize(position);
            }
        };
        mOEMRecyclerView.setLayoutStyle(oemLayoutStyle);
    }

    @Override
    public void setPadding(int left, int top, int right, int bottom) {
        mOEMRecyclerView.setPadding(left, top, right, bottom);
    }

    @Override
    public void setPaddingRelative(int start, int top, int end, int bottom) {
        mOEMRecyclerView.setPaddingRelative(start, top, end, bottom);
    }

    @Override
    public void setClipToPadding(boolean clipToPadding) {
        mOEMRecyclerView.setClipToPadding(clipToPadding);
    }
}
