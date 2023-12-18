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
import com.android.wallpaper.model.WorkspaceViewModel;

import java.util.List;

/** Interface for carry {@link CustomizationSectionController}s. */
public interface CustomizationSections {

    /**
     * Gets a new instance of the section controller list.
     *
     * Note that the section views will be displayed by the list ordering.
     *
     * <p>Don't keep the section controllers as singleton since they contain views.
     */
    List<CustomizationSectionController<?>> getAllSectionControllers(
            Activity activity,
            LifecycleOwner lifecycleOwner,
            WallpaperColorsViewModel wallpaperColorsViewModel,
            WorkspaceViewModel workspaceViewModel,
            PermissionRequester permissionRequester,
            WallpaperPreviewNavigator wallpaperPreviewNavigator,
            CustomizationSectionNavigationController sectionNavigationController,
            @Nullable Bundle savedInstanceState);
}
