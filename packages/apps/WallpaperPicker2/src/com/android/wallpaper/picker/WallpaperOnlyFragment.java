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

import com.android.wallpaper.model.CustomizationSectionController;
import com.android.wallpaper.model.WallpaperSectionController;

import java.util.List;
import java.util.stream.Collectors;

/** The Fragment UI for wallpaper only section. */
public class WallpaperOnlyFragment extends CustomizationPickerFragment {

    /** Initiates WallpaperOnlyFragment instance. */
    public static WallpaperOnlyFragment newInstance(CharSequence title) {
        WallpaperOnlyFragment fragment = new WallpaperOnlyFragment();
        fragment.setArguments(AppbarFragment.createArguments(title));
        return fragment;
    }

    @Override
    protected List<CustomizationSectionController<?>> getAvailableSections(
            List<CustomizationSectionController<?>> controllers) {
        List<CustomizationSectionController<?>> wallpaperOnlySections = controllers.stream()
                .filter(controller -> controller instanceof WallpaperSectionController)
                .collect(Collectors.toList());
        return super.getAvailableSections(wallpaperOnlySections);
    }
}
