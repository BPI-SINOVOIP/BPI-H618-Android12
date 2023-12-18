package com.android.customization.module;

import android.app.Activity;
import android.os.Bundle;

import androidx.annotation.Nullable;
import androidx.lifecycle.LifecycleOwner;

import com.android.customization.model.grid.GridOptionsManager;
import com.android.customization.model.grid.GridSectionController;
import com.android.customization.model.mode.DarkModeSectionController;
import com.android.customization.model.themedicon.ThemedIconSectionController;
import com.android.customization.model.themedicon.ThemedIconSwitchProvider;
import com.android.wallpaper.model.CustomizationSectionController;
import com.android.wallpaper.model.CustomizationSectionController.CustomizationSectionNavigationController;
import com.android.wallpaper.model.PermissionRequester;
import com.android.wallpaper.model.WallpaperColorsViewModel;
import com.android.wallpaper.model.WallpaperPreviewNavigator;
import com.android.wallpaper.model.WallpaperSectionController;
import com.android.wallpaper.model.WorkspaceViewModel;
import com.android.wallpaper.module.CustomizationSections;

import java.util.ArrayList;
import java.util.List;

/** {@link CustomizationSections} for the customization picker. */
public final class DefaultCustomizationSections implements CustomizationSections {

    @Override
    public List<CustomizationSectionController<?>> getAllSectionControllers(Activity activity,
            LifecycleOwner lifecycleOwner, WallpaperColorsViewModel wallpaperColorsViewModel,
            WorkspaceViewModel workspaceViewModel, PermissionRequester permissionRequester,
            WallpaperPreviewNavigator wallpaperPreviewNavigator,
            CustomizationSectionNavigationController sectionNavigationController,
            @Nullable Bundle savedInstanceState) {
        List<CustomizationSectionController<?>> sectionControllers = new ArrayList<>();

        // Wallpaper section.
        sectionControllers.add(new WallpaperSectionController(
                activity, lifecycleOwner, permissionRequester, wallpaperColorsViewModel,
                workspaceViewModel, sectionNavigationController, wallpaperPreviewNavigator,
                savedInstanceState));

        // Dark/Light theme section.
        sectionControllers.add(new DarkModeSectionController(activity,
                lifecycleOwner.getLifecycle()));

        // Themed app icon section.
        sectionControllers.add(new ThemedIconSectionController(
                ThemedIconSwitchProvider.getInstance(activity), workspaceViewModel,
                savedInstanceState));

        // App grid section.
        sectionControllers.add(new GridSectionController(
                GridOptionsManager.getInstance(activity), sectionNavigationController));

        return sectionControllers;
    }
}
