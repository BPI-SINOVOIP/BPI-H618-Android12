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
package com.android.customization.model.grid;

import android.content.Context;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.widget.TextView;

import androidx.annotation.Nullable;

import com.android.customization.model.CustomizationManager.OptionsFetchedListener;
import com.android.customization.picker.grid.GridFragment;
import com.android.customization.picker.grid.GridSectionView;
import com.android.wallpaper.R;
import com.android.wallpaper.model.CustomizationSectionController;

import java.util.List;

/** A {@link CustomizationSectionController} for app grid. */
public class GridSectionController implements CustomizationSectionController<GridSectionView> {

    private static final String TAG = "GridSectionController";

    private final GridOptionsManager mGridOptionsManager;
    private final CustomizationSectionNavigationController mSectionNavigationController;

    public GridSectionController(GridOptionsManager gridOptionsManager,
            CustomizationSectionNavigationController sectionNavigationController) {
        mGridOptionsManager = gridOptionsManager;
        mSectionNavigationController = sectionNavigationController;
    }

    @Override
    public boolean isAvailable(Context context) {
        return mGridOptionsManager.isAvailable();
    }

    @Override
    public GridSectionView createView(Context context) {
        GridSectionView gridSectionView = (GridSectionView) LayoutInflater.from(context)
                .inflate(R.layout.grid_section_view, /* root= */ null);
        TextView sectionDescription = gridSectionView.findViewById(R.id.grid_section_description);
        View sectionTile = gridSectionView.findViewById(R.id.grid_section_tile);

        // Fetch grid options to show currently set grid.
        mGridOptionsManager.fetchOptions(new OptionsFetchedListener<GridOption>() {
            @Override
            public void onOptionsLoaded(List<GridOption> options) {
                sectionDescription.setText(getActiveOption(options).getTitle());
            }

            @Override
            public void onError(@Nullable Throwable throwable) {
                if (throwable != null) {
                    Log.e(TAG, "Error loading grid options", throwable);
                }
                sectionDescription.setText(R.string.something_went_wrong);
                sectionTile.setVisibility(View.GONE);
            }
        }, /* reload= */ true);

        gridSectionView.setOnClickListener(v -> mSectionNavigationController.navigateTo(
                GridFragment.newInstance(context.getString(R.string.grid_title))));

        return gridSectionView;
    }

    private GridOption getActiveOption(List<GridOption> options) {
        return options.stream()
                .filter(option -> option.isActive(mGridOptionsManager))
                .findAny()
                // For development only, as there should always be a grid set.
                .orElse(options.get(0));
    }
}
