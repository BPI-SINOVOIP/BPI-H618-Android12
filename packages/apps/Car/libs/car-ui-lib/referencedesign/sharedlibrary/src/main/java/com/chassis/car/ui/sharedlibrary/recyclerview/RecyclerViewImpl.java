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
package com.chassis.car.ui.sharedlibrary.recyclerview;

import android.content.Context;
import android.view.View;

import androidx.annotation.NonNull;
import androidx.recyclerview.widget.GridLayoutManager;
import androidx.recyclerview.widget.GridLayoutManager.SpanSizeLookup;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import com.android.car.ui.sharedlibrary.oemapis.recyclerview.AdapterOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.LayoutStyleOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.OnScrollListenerOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewAttributesOEMV1;
import com.android.car.ui.sharedlibrary.oemapis.recyclerview.RecyclerViewOEMV1;

import java.util.ArrayList;
import java.util.List;

/**
 * Reference OEM implementation for RecyclerView
 */
public final class RecyclerViewImpl extends RecyclerView implements RecyclerViewOEMV1 {

    @NonNull
    private List<OnScrollListenerOEMV1> mScrollListeners = new ArrayList<>();

    @NonNull
    private RecyclerView.OnScrollListener mOnScrollListener = new RecyclerView.OnScrollListener() {
        @Override
        public void onScrolled(@NonNull RecyclerView recyclerView, int dx, int dy) {
            for (OnScrollListenerOEMV1 listener: mScrollListeners) {
                listener.onScrolled((RecyclerViewOEMV1) recyclerView, dx, dy);
            }
        }

        @Override
        public void onScrollStateChanged(RecyclerView recyclerView, int newState) {
            for (OnScrollListenerOEMV1 listener: mScrollListeners) {
                listener.onScrollStateChanged((RecyclerViewOEMV1) recyclerView, newState);
            }
        }
    };

    public RecyclerViewImpl(@NonNull Context context) {
        super(context);
    }

    public RecyclerViewImpl(Context context, RecyclerViewAttributesOEMV1 attrs) {
        super(context);
        setLayoutStyle(attrs.getLayoutStyle());
    }

    @Override
    public void setAdapter(AdapterOEMV1 adapterV1) {
        if (adapterV1 == null) {
            super.setAdapter(null);
        } else {
            super.setAdapter(new AdapterWrapper(adapterV1));
        }
    }

    @Override
    public void addOnScrollListener(OnScrollListenerOEMV1 listener) {
        if (listener == null) {
            return;
        }
        if (mScrollListeners.isEmpty()) {
            super.addOnScrollListener(mOnScrollListener);
        }
        mScrollListeners.add(listener);
    }

    @Override
    public void removeOnScrollListener(OnScrollListenerOEMV1 listener) {
        if (listener == null) {
            return;
        }
        mScrollListeners.remove(listener);
        if (mScrollListeners.isEmpty()) {
            super.removeOnScrollListener(mOnScrollListener);
        }
    }

    @Override
    public void clearOnScrollListeners() {
        if (mScrollListeners != null) {
            mScrollListeners.clear();
            super.clearOnScrollListeners();
        }
    }

    @Override
    public void scrollToPosition(int position) {
        super.scrollToPosition(position);
    }

    @Override
    public void smoothScrollBy(int dx, int dy) {
        super.smoothScrollBy(dx, dy);
    }

    @Override
    public void smoothScrollToPosition(int position) {
        super.smoothScrollToPosition(position);
    }

    @Override
    public void setHasFixedSize(boolean hasFixedSize) {
        super.setHasFixedSize(hasFixedSize);
    }

    @Override
    public boolean hasFixedSize() {
        return super.hasFixedSize();
    }

    @Override
    public void setLayoutStyle(LayoutStyleOEMV1 layoutStyle) {
        if (layoutStyle.getLayoutType() == LayoutStyleOEMV1.LAYOUT_TYPE_LINEAR) {
            setLayoutManager(new LinearLayoutManager(getContext(),
                    layoutStyle.getOrientation(),
                    layoutStyle.getReverseLayout()));
        } else {
            setLayoutManager(new GridLayoutManager(getContext(),
                    layoutStyle.getSpanCount(),
                    layoutStyle.getOrientation(),
                    layoutStyle.getReverseLayout()));
            if (layoutStyle.getSpanSizeLookup() != null) {
                ((GridLayoutManager) getLayoutManager()).setSpanSizeLookup(new SpanSizeLookup() {
                    @Override
                    public int getSpanSize(int position) {
                        return layoutStyle.getSpanSizeLookup().getSpanSize(position);
                    }
                });
            }
        }
    }

    public View getView() {
        return this;
    }

    @Override
    public View getContainer() {
        return this;
    }
}
