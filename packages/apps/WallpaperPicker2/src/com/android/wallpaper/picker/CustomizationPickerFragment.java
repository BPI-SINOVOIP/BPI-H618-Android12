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
package com.android.wallpaper.picker;

import android.content.Context;
import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;

import androidx.annotation.Nullable;
import androidx.core.widget.NestedScrollView;
import androidx.fragment.app.Fragment;
import androidx.fragment.app.FragmentManager;
import androidx.lifecycle.ViewModelProvider;

import com.android.wallpaper.R;
import com.android.wallpaper.model.CustomizationSectionController;
import com.android.wallpaper.model.CustomizationSectionController.CustomizationSectionNavigationController;
import com.android.wallpaper.model.PermissionRequester;
import com.android.wallpaper.model.WallpaperColorsViewModel;
import com.android.wallpaper.model.WallpaperPreviewNavigator;
import com.android.wallpaper.model.WorkspaceViewModel;
import com.android.wallpaper.module.CustomizationSections;
import com.android.wallpaper.module.InjectorProvider;
import com.android.wallpaper.util.ActivityUtils;

import java.util.ArrayList;
import java.util.List;
import java.util.stream.Collectors;

/** The Fragment UI for customization sections. */
public class CustomizationPickerFragment extends AppbarFragment implements
        CustomizationSectionNavigationController {

    private static final String TAG = "CustomizationPickerFragment";
    private static final String SCROLL_POSITION_Y = "SCROLL_POSITION_Y";

    // Note that the section views will be displayed by the list ordering.
    private final List<CustomizationSectionController<?>> mSectionControllers = new ArrayList<>();
    private NestedScrollView mNestedScrollView;
    @Nullable private Bundle mBackStackSavedInstanceState;

    /** Initiates CustomizationPickerFragment instance. */
    public static CustomizationPickerFragment newInstance(CharSequence title) {
        CustomizationPickerFragment fragment = new CustomizationPickerFragment();
        fragment.setArguments(AppbarFragment.createArguments(title));
        return fragment;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        final View view = inflater.inflate(R.layout.collapsing_toolbar_base_layout,
                container, /* attachToRoot= */ false);
        setContentView(view, R.layout.fragment_customization_picker);
        setUpToolbar(view, ActivityUtils.isLaunchedFromSettingsRelated(getActivity().getIntent()));

        ViewGroup sectionContainer = view.findViewById(R.id.section_container);
        sectionContainer.setOnApplyWindowInsetsListener((v, windowInsets) -> {
            v.setPadding(
                    v.getPaddingLeft(),
                    v.getPaddingTop(),
                    v.getPaddingRight(),
                    windowInsets.getSystemWindowInsetBottom());
            return windowInsets.consumeSystemWindowInsets();
        });
        mNestedScrollView = view.findViewById(R.id.scroll_container);

        if (mBackStackSavedInstanceState != null) {
            savedInstanceState = mBackStackSavedInstanceState;
            mBackStackSavedInstanceState = null;
        }

        initSections(savedInstanceState);
        mSectionControllers.forEach(controller ->
                mNestedScrollView.post(() -> {
                            final Context context = getContext();
                            if (context == null) {
                                Log.w(TAG, "Adding section views with null context");
                                return;
                            }
                            sectionContainer.addView(controller.createView(context));
                        }
                )
        );
        final Bundle savedInstanceStateRef = savedInstanceState;
        // Post it to the end of adding views to ensure restoring view state the last task.
        mNestedScrollView.post(() ->
                restoreViewState(savedInstanceStateRef)
        );
        return view;
    }

    private void setContentView(View view, int layoutResId) {
        final ViewGroup parent = view.findViewById(R.id.content_frame);
        if (parent != null) {
            parent.removeAllViews();
        }
        LayoutInflater.from(view.getContext()).inflate(layoutResId, parent);
    }

    private void restoreViewState(@Nullable Bundle savedInstanceState) {
        if (savedInstanceState != null) {
            mNestedScrollView.post(() ->
                    mNestedScrollView.setScrollY(savedInstanceState.getInt(SCROLL_POSITION_Y)));
        }
    }

    @Override
    public void onSaveInstanceState(Bundle savedInstanceState) {
        onSaveInstanceStateInternal(savedInstanceState);
        super.onSaveInstanceState(savedInstanceState);
    }

    @Override
    protected int getToolbarId() {
        return R.id.action_bar;
    }

    @Override
    public boolean onBackPressed() {
        // TODO(b/191120122) Improve glitchy animation in Settings.
        if (ActivityUtils.isLaunchedFromSettingsSearch(getActivity().getIntent())) {
            mSectionControllers.forEach(CustomizationSectionController::onTransitionOut);
        }
        return super.onBackPressed();
    }

    @Override
    public void onDestroyView() {
        // When add to back stack, #onDestroyView would be called, but #onDestroy wouldn't. So
        // storing the state in variable to restore when back to foreground. If it's not a back
        // stack case (i,e, config change), the variable would not be retained, see
        // https://developer.android.com/guide/fragments/saving-state.
        mBackStackSavedInstanceState = new Bundle();
        onSaveInstanceStateInternal(mBackStackSavedInstanceState);

        mSectionControllers.forEach(CustomizationSectionController::release);
        mSectionControllers.clear();
        super.onDestroyView();
    }

    @Override
    public void navigateTo(Fragment fragment) {
        FragmentManager fragmentManager = getActivity().getSupportFragmentManager();
        fragmentManager
                .beginTransaction()
                .replace(R.id.fragment_container, fragment)
                .addToBackStack(null)
                .commit();
        fragmentManager.executePendingTransactions();
    }

    /** Saves state of the fragment. */
    private void onSaveInstanceStateInternal(Bundle savedInstanceState) {
        if (mNestedScrollView != null) {
            savedInstanceState.putInt(SCROLL_POSITION_Y, mNestedScrollView.getScrollY());
        }
        mSectionControllers.forEach(c -> c.onSaveInstanceState(savedInstanceState));
    }

    private void initSections(@Nullable Bundle savedInstanceState) {
        // Release and clear if any.
        mSectionControllers.forEach(CustomizationSectionController::release);
        mSectionControllers.clear();

        WallpaperColorsViewModel wcViewModel = new ViewModelProvider(getActivity())
                .get(WallpaperColorsViewModel.class);
        WorkspaceViewModel workspaceViewModel = new ViewModelProvider(getActivity())
                .get(WorkspaceViewModel.class);

        CustomizationSections sections = InjectorProvider.getInjector().getCustomizationSections();
        List<CustomizationSectionController<?>> allSectionControllers =
                sections.getAllSectionControllers(getActivity(), getViewLifecycleOwner(),
                        wcViewModel, workspaceViewModel, getPermissionRequester(),
                        getWallpaperPreviewNavigator(), this, savedInstanceState);

        mSectionControllers.addAll(getAvailableSections(allSectionControllers));
    }

    protected List<CustomizationSectionController<?>> getAvailableSections(
            List<CustomizationSectionController<?>> controllers) {
        return controllers.stream()
                .filter(controller -> {
                    if(controller.isAvailable(getContext())) {
                        return true;
                    } else {
                        controller.release();
                        Log.d(TAG, "Section is not available: " + controller);
                        return false;
                    }})
                .collect(Collectors.toList());
    }

    private PermissionRequester getPermissionRequester() {
        return (PermissionRequester) getActivity();
    }

    private WallpaperPreviewNavigator getWallpaperPreviewNavigator() {
        return (WallpaperPreviewNavigator) getActivity();
    }
}
