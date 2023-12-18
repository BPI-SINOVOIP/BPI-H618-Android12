/*
 * Copyright (C) 2018 The Android Open Source Project
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
package com.android.customization.picker.grid;

import static com.android.wallpaper.widget.BottomActionBar.BottomAction.APPLY_TEXT;

import android.os.Bundle;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.SurfaceView;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.core.widget.ContentLoadingProgressBar;
import androidx.recyclerview.widget.RecyclerView;

import com.android.customization.model.CustomizationManager.Callback;
import com.android.customization.model.CustomizationManager.OptionsFetchedListener;
import com.android.customization.model.CustomizationOption;
import com.android.customization.model.grid.GridOption;
import com.android.customization.model.grid.GridOptionsManager;
import com.android.customization.module.ThemesUserEventLogger;
import com.android.customization.picker.WallpaperPreviewer;
import com.android.customization.widget.OptionSelectorController;
import com.android.customization.widget.OptionSelectorController.CheckmarkStyle;
import com.android.wallpaper.R;
import com.android.wallpaper.model.WallpaperInfo;
import com.android.wallpaper.module.CurrentWallpaperInfoFactory;
import com.android.wallpaper.module.InjectorProvider;
import com.android.wallpaper.picker.AppbarFragment;
import com.android.wallpaper.util.LaunchUtils;
import com.android.wallpaper.widget.BottomActionBar;

import com.bumptech.glide.Glide;

import java.util.List;

/**
 * Fragment that contains the UI for selecting and applying a GridOption.
 */
public class GridFragment extends AppbarFragment {

    private static final String TAG = "GridFragment";
    private static final String KEY_STATE_SELECTED_OPTION = "GridFragment.selectedOption";
    private static final String KEY_STATE_BOTTOM_ACTION_BAR_VISIBLE =
            "GridFragment.bottomActionBarVisible";

    public static GridFragment newInstance(CharSequence title) {
        GridFragment fragment = new GridFragment();
        fragment.setArguments(AppbarFragment.createArguments(title));
        return fragment;
    }

    private WallpaperInfo mHomeWallpaper;
    private RecyclerView mOptionsContainer;
    private OptionSelectorController<GridOption> mOptionsController;
    private GridOptionsManager mGridManager;
    private GridOption mSelectedOption;
    private ContentLoadingProgressBar mLoading;
    private View mContent;
    private View mError;
    private BottomActionBar mBottomActionBar;
    private ThemesUserEventLogger mEventLogger;
    private GridOptionPreviewer mGridOptionPreviewer;

    private final Callback mApplyGridCallback = new Callback() {
        @Override
        public void onSuccess() {
            Toast.makeText(getContext(), R.string.applied_grid_msg, Toast.LENGTH_SHORT).show();
            getActivity().overridePendingTransition(R.anim.fade_in, R.anim.fade_out);
            getActivity().finish();

            // Go back to launcher home
            LaunchUtils.launchHome(getContext());
        }

        @Override
        public void onError(@Nullable Throwable throwable) {
            // Since we disabled it when clicked apply button.
            mBottomActionBar.enableActions();
            mBottomActionBar.hide();
            //TODO(chihhangchuang): handle
        }
    };

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = inflater.inflate(
                R.layout.fragment_grid_picker, container, /* attachToRoot */ false);
        setUpToolbar(view);
        mContent = view.findViewById(R.id.content_section);
        mOptionsContainer = view.findViewById(R.id.options_container);
        mLoading = view.findViewById(R.id.loading_indicator);
        mError = view.findViewById(R.id.error_section);

        // For nav bar edge-to-edge effect.
        view.setOnApplyWindowInsetsListener((v, windowInsets) -> {
            v.setPadding(
                    v.getPaddingLeft(),
                    windowInsets.getSystemWindowInsetTop(),
                    v.getPaddingRight(),
                    windowInsets.getSystemWindowInsetBottom());
            return windowInsets.consumeSystemWindowInsets();
        });

        // Clear memory cache whenever grid fragment view is being loaded.
        Glide.get(getContext()).clearMemory();

        mGridManager = GridOptionsManager.getInstance(getContext());
        mEventLogger = (ThemesUserEventLogger) InjectorProvider.getInjector()
                .getUserEventLogger(getContext());
        setUpOptions(savedInstanceState);

        SurfaceView wallpaperSurface = view.findViewById(R.id.wallpaper_preview_surface);
        WallpaperPreviewer wallpaperPreviewer = new WallpaperPreviewer(getLifecycle(),
                getActivity(), view.findViewById(R.id.wallpaper_preview_image), wallpaperSurface);
        // Loads current Wallpaper.
        CurrentWallpaperInfoFactory factory = InjectorProvider.getInjector()
                .getCurrentWallpaperFactory(getContext().getApplicationContext());
        factory.createCurrentWallpaperInfos((homeWallpaper, lockWallpaper, presentationMode) -> {
            mHomeWallpaper = homeWallpaper;
            wallpaperPreviewer.setWallpaper(mHomeWallpaper, /* listener= */ null);
        }, false);

        mGridOptionPreviewer = new GridOptionPreviewer(mGridManager,
                view.findViewById(R.id.grid_preview_container));

        return view;
    }

    @Override
    public void onDestroy() {
        super.onDestroy();
        if (mGridOptionPreviewer != null) {
            mGridOptionPreviewer.release();
        }
    }

    @Override
    public void onSaveInstanceState(@NonNull Bundle outState) {
        super.onSaveInstanceState(outState);
        if (mSelectedOption != null) {
            outState.putParcelable(KEY_STATE_SELECTED_OPTION, mSelectedOption);
        }
        if (mBottomActionBar != null) {
            outState.putBoolean(KEY_STATE_BOTTOM_ACTION_BAR_VISIBLE, mBottomActionBar.isVisible());
        }
    }

    @Override
    protected void onBottomActionBarReady(BottomActionBar bottomActionBar) {
        super.onBottomActionBarReady(bottomActionBar);
        mBottomActionBar = bottomActionBar;
        mBottomActionBar.showActionsOnly(APPLY_TEXT);
        mBottomActionBar.setActionClickListener(APPLY_TEXT, v -> applyGridOption(mSelectedOption));
    }

    private void applyGridOption(GridOption gridOption) {
        mBottomActionBar.disableActions();
        mGridManager.apply(gridOption, mApplyGridCallback);
    }

    private void setUpOptions(@Nullable Bundle savedInstanceState) {
        hideError();
        mLoading.show();
        mGridManager.fetchOptions(new OptionsFetchedListener<GridOption>() {
            @Override
            public void onOptionsLoaded(List<GridOption> options) {
                mLoading.hide();
                mOptionsController = new OptionSelectorController<>(
                        mOptionsContainer, options, /* useGrid= */ false, CheckmarkStyle.CENTER);
                mOptionsController.initOptions(mGridManager);

                // Find the selected Grid option.
                GridOption previouslySelectedOption = null;
                if (savedInstanceState != null) {
                    previouslySelectedOption = findEquivalent(
                            options, savedInstanceState.getParcelable(KEY_STATE_SELECTED_OPTION));
                }
                mSelectedOption = previouslySelectedOption != null
                        ? previouslySelectedOption
                        : getActiveOption(options);

                mOptionsController.setSelectedOption(mSelectedOption);
                onOptionSelected(mSelectedOption);
                restoreBottomActionBarVisibility(savedInstanceState);

                mOptionsController.addListener(selectedOption -> {
                    onOptionSelected(selectedOption);
                    mBottomActionBar.show();
                });
            }

            @Override
            public void onError(@Nullable Throwable throwable) {
                if (throwable != null) {
                    Log.e(TAG, "Error loading grid options", throwable);
                }
                showError();
            }
        }, /*reload= */ true);
    }

    private GridOption getActiveOption(List<GridOption> options) {
        return options.stream()
                .filter(option -> option.isActive(mGridManager))
                .findAny()
                // For development only, as there should always be a grid set.
                .orElse(options.get(0));
    }

    @Nullable
    private GridOption findEquivalent(List<GridOption> options, GridOption target) {
        return options.stream()
                .filter(option -> option.equals(target))
                .findAny()
                .orElse(null);
    }

    private void hideError() {
        mContent.setVisibility(View.VISIBLE);
        mError.setVisibility(View.GONE);
    }

    private void showError() {
        mLoading.hide();
        mContent.setVisibility(View.GONE);
        mError.setVisibility(View.VISIBLE);
    }

    private void onOptionSelected(CustomizationOption selectedOption) {
        mSelectedOption = (GridOption) selectedOption;
        mEventLogger.logGridSelected(mSelectedOption);
        mGridOptionPreviewer.setGridOption(mSelectedOption);
    }

    private void restoreBottomActionBarVisibility(@Nullable Bundle savedInstanceState) {
        boolean isBottomActionBarVisible = savedInstanceState != null
                && savedInstanceState.getBoolean(KEY_STATE_BOTTOM_ACTION_BAR_VISIBLE);
        if (isBottomActionBarVisible) {
            mBottomActionBar.show();
        } else {
            mBottomActionBar.hide();
        }
    }
}
