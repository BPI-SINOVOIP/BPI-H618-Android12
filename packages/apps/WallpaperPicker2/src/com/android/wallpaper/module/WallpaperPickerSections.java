package com.android.wallpaper.module;

import android.app.Activity;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;

import com.android.wallpaper.model.CustomizationSectionController;
import com.android.wallpaper.model.CustomizationSectionController.CustomizationSectionNavigationController;
import com.android.wallpaper.model.PermissionRequester;
import com.android.wallpaper.model.WallpaperColorsViewModel;
import com.android.wallpaper.model.WallpaperPreviewNavigator;
import com.android.wallpaper.model.WallpaperSectionController;
import com.android.wallpaper.model.WorkspaceViewModel;

import java.util.ArrayList;
import java.util.List;

/** {@link CustomizationSections} for the wallpaper picker. */
public final class WallpaperPickerSections implements CustomizationSections {

    @Override
    public List<CustomizationSectionController<?>> getAllSectionControllers(Activity activity,
            LifecycleOwner lifecycleOwner, WallpaperColorsViewModel wallpaperColorsViewModel,
            WorkspaceViewModel workspaceViewModel, PermissionRequester permissionRequester,
            WallpaperPreviewNavigator wallpaperPreviewNavigator,
            CustomizationSectionNavigationController sectionNavigationController,
            @Nullable Bundle savedInstanceState) {
        List<CustomizationSectionController<?>> sectionControllers = new ArrayList<>();

        sectionControllers.add(new WallpaperSectionController(
                activity, lifecycleOwner, permissionRequester, wallpaperColorsViewModel,
                workspaceViewModel, sectionNavigationController, wallpaperPreviewNavigator,
                savedInstanceState));

        return sectionControllers;
    }
}
