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

package com.android.car.messenger.core.ui.base;

import android.content.Context;
import android.os.Bundle;
import androidx.fragment.app.Fragment;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.CallSuper;
import androidx.annotation.LayoutRes;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.content.ContextCompat;

import com.android.car.messenger.R;
import com.android.car.messenger.core.ui.shared.LoadingFrameLayout;
import com.android.car.ui.baselayout.Insets;
import com.android.car.ui.baselayout.InsetsChangedListener;
import com.android.car.ui.core.CarUi;
import com.android.car.ui.recyclerview.CarUiRecyclerView;
import com.android.car.ui.toolbar.Toolbar.State;
import com.android.car.ui.toolbar.ToolbarController;

/**
 * Base fragment that inflates a {@link RecyclerView}. It handles the top offset for first row item
 * so the list can scroll underneath the top bar.
 */
public class MessageListBaseFragment extends Fragment implements InsetsChangedListener {

    @NonNull protected LoadingFrameLayout mLoadingFrameLayout;
    @NonNull private CarUiRecyclerView mRecyclerView;
    @Nullable protected ToolbarController mToolbar;

    @Override
    public View onCreateView(
            LayoutInflater inflater, ViewGroup container, Bundle savedInstanceState) {
        View view = inflater.inflate(getLayoutResource(), container, false);
        mLoadingFrameLayout = view.findViewById(R.id.loading_frame_layout);
        mRecyclerView = view.requireViewById(R.id.list_view);
        mRecyclerView.setLayoutManager(createLayoutManager());
        return view;
    }

    /** Layout resource for this fragment. It must contains a RecyclerView with id list_view. */
    @LayoutRes
    protected int getLayoutResource() {
        return R.layout.loading_list_fragment;
    }

    /**
     * Creates the layout manager for the recycler view. Default is a {@link LinearLayoutManager}.
     * Child inheriting from this fragment can override to create a different layout manager.
     */
    @NonNull
    protected RecyclerView.LayoutManager createLayoutManager() {
        return new LinearLayoutManager(getContext());
    }

    /** Returns the {@link RecyclerView} instance. */
    @NonNull
    protected CarUiRecyclerView getRecyclerView() {
        return mRecyclerView;
    }

    @CallSuper
    @Override
    public void onViewCreated(@NonNull View view, @Nullable Bundle savedInstanceState) {
        super.onViewCreated(view, savedInstanceState);
        mToolbar = CarUi.getToolbar(requireActivity());
        // Null check for unit tests to pass
        if (mToolbar != null) {
            setupToolbar(mToolbar);
        }
        Insets insets = CarUi.getInsets(requireActivity());
        // Null check for unit tests to pass
        if (insets != null) {
            onCarUiInsetsChanged(insets);
        }
    }

    /** Customizes the tool bar. Can be overridden in subclasses. */
    protected void setupToolbar(@NonNull ToolbarController toolbar) {
        Context context = getContext();
        if (context == null) {
            return;
        }
        toolbar.setTitle(R.string.app_name);
        toolbar.setState(getToolbarState());
        toolbar.setLogo(
                getToolbarState() == State.HOME
                        ? ContextCompat.getDrawable(context, context.getApplicationInfo().icon)
                        : null);
    }

    @NonNull
    protected State getToolbarState() {
        return State.HOME;
    }

    @Override
    public void onCarUiInsetsChanged(Insets insets) {
        int listTopPadding =
                requireContext().getResources().getDimensionPixelSize(R.dimen.list_top_padding);
        mRecyclerView.setPadding(0, insets.getTop() + listTopPadding, 0, insets.getBottom());
    }
}
