/*
 * Copyright (C) 2019 The Android Open Source Project
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
package com.android.customization.module;

import static com.android.customization.model.ResourceConstants.OVERLAY_CATEGORY_COLOR;
import static com.android.customization.model.ResourceConstants.OVERLAY_CATEGORY_FONT;
import static com.android.customization.model.ResourceConstants.OVERLAY_CATEGORY_SHAPE;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__ACTION__APP_LAUNCHED;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_CROP_AND_SET_ACTION;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_DEEP_LINK;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_LAUNCHER;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_LAUNCH_ICON;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_PREFERENCE_UNSPECIFIED;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_SETTINGS;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_SETTINGS_SEARCH;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_SUW;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_TIPS;
import static com.android.systemui.shared.system.SysUiStatsLog.STYLE_UI_CHANGED;
import static com.android.wallpaper.util.LaunchSourceUtils.LAUNCH_SETTINGS_SEARCH;
import static com.android.wallpaper.util.LaunchSourceUtils.LAUNCH_SOURCE_DEEP_LINK;
import static com.android.wallpaper.util.LaunchSourceUtils.LAUNCH_SOURCE_LAUNCHER;
import static com.android.wallpaper.util.LaunchSourceUtils.LAUNCH_SOURCE_SETTINGS;
import static com.android.wallpaper.util.LaunchSourceUtils.LAUNCH_SOURCE_SUW;
import static com.android.wallpaper.util.LaunchSourceUtils.LAUNCH_SOURCE_TIPS;
import static com.android.wallpaper.util.LaunchSourceUtils.WALLPAPER_LAUNCH_SOURCE;

import android.app.WallpaperManager;
import android.content.Intent;
import android.stats.style.StyleEnums;

import androidx.annotation.Nullable;

import com.android.customization.model.clock.Clockface;
import com.android.customization.model.grid.GridOption;
import com.android.customization.model.theme.ThemeBundle;
import com.android.systemui.shared.system.SysUiStatsLog;
import com.android.wallpaper.module.NoOpUserEventLogger;

import java.util.Map;
import java.util.Objects;

/**
 * StatsLog-backed implementation of {@link ThemesUserEventLogger}.
 */
public class StatsLogUserEventLogger extends NoOpUserEventLogger implements ThemesUserEventLogger {

    private static final String TAG = "StatsLogUserEventLogger";

    @Override
    public void logAppLaunched(Intent launchSource) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, STYLE_UICHANGED__ACTION__APP_LAUNCHED, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, getAppLaunchSource(launchSource));
    }

    @Override
    public void logResumed(boolean provisioned, boolean wallpaper) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.ONRESUME, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                0);
    }

    @Override
    public void logStopped() {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.ONSTOP, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
    }

    @Override
    public void logActionClicked(String collectionId, int actionLabelResId) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.WALLPAPER_EXPLORE, 0, 0, 0, 0, 0,
                getCollectionIdHashCode(collectionId), 0, 0, 0, 0, 0);
    }

    @Override
    public void logIndividualWallpaperSelected(String collectionId) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.WALLPAPER_SELECT, 0, 0, 0, 0, 0,
                getCollectionIdHashCode(collectionId), 0, 0, 0, 0, 0);
    }

    @Override
    public void logCategorySelected(String collectionId) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.WALLPAPER_OPEN_CATEGORY,
                0, 0, 0, 0, 0,
                getCollectionIdHashCode(collectionId),
                0, 0, 0, 0, 0);
    }

    @Override
    public void logLiveWallpaperInfoSelected(String collectionId, @Nullable String wallpaperId) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.LIVE_WALLPAPER_INFO_SELECT,
                0, 0, 0, 0, 0,
                getCollectionIdHashCode(collectionId),
                wallpaperId != null ? wallpaperId.hashCode() : 0,
                0, 0, 0, 0);
    }

    @Override
    public void logLiveWallpaperCustomizeSelected(String collectionId,
            @Nullable String wallpaperId) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.LIVE_WALLPAPER_CUSTOMIZE_SELECT,
                0, 0, 0, 0, 0,
                getCollectionIdHashCode(collectionId),
                wallpaperId != null ? wallpaperId.hashCode() : 0,
                0, 0, 0, 0);
    }

    @Override
    public void logWallpaperSet(String collectionId, @Nullable String wallpaperId) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.WALLPAPER_APPLIED,
                0, 0, 0, 0, 0,
                getCollectionIdHashCode(collectionId),
                wallpaperId != null ? wallpaperId.hashCode() : 0,
                0, 0, 0, 0);
    }

    @Nullable
    private String getThemePackage(ThemeBundle theme, String category) {
        Map<String, String> packages = theme.getPackagesByCategory();
        return packages.get(category);
    }

    @Override
    public void logThemeSelected(ThemeBundle theme, boolean isCustomTheme) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.PICKER_SELECT,
                Objects.hashCode(getThemePackage(theme, OVERLAY_CATEGORY_COLOR)),
                Objects.hashCode(getThemePackage(theme,OVERLAY_CATEGORY_FONT)),
                Objects.hashCode(getThemePackage(theme, OVERLAY_CATEGORY_SHAPE)),
                0, 0, 0, 0, 0, 0, 0, 0);
    }

    @Override
    public void logThemeApplied(ThemeBundle theme, boolean isCustomTheme) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.PICKER_APPLIED,
                Objects.hashCode(getThemePackage(theme, OVERLAY_CATEGORY_COLOR)),
                Objects.hashCode(getThemePackage(theme,OVERLAY_CATEGORY_FONT)),
                Objects.hashCode(getThemePackage(theme, OVERLAY_CATEGORY_SHAPE)),
                0, 0, 0, 0, 0, 0, 0, 0);
    }

    @Override
    public void logColorApplied(int action, int colorIndex) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, action,
                0, 0, 0, 0, 0, 0, 0,
                colorIndex,
                0, 0, 0);
    }

    @Override
    public void logClockSelected(Clockface clock) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.PICKER_SELECT,
                0, 0, 0,
                Objects.hashCode(clock.getId()),
                0, 0, 0, 0, 0, 0, 0);
    }

    @Override
    public void logClockApplied(Clockface clock) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.PICKER_APPLIED,
                0, 0, 0,
                Objects.hashCode(clock.getId()),
                0, 0, 0, 0, 0, 0, 0);
    }

    @Override
    public void logGridSelected(GridOption grid) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.PICKER_SELECT,
                0, 0, 0, 0,
                grid.cols,
                0, 0, 0, 0, 0, 0);
    }

    @Override
    public void logGridApplied(GridOption grid) {
        SysUiStatsLog.write(STYLE_UI_CHANGED, StyleEnums.PICKER_APPLIED,
                0, 0, 0, 0,
                grid.cols,
                0, 0, 0, 0, 0, 0);
    }

    private int getAppLaunchSource(Intent launchSource) {
        if (launchSource.hasExtra(WALLPAPER_LAUNCH_SOURCE)) {
            switch (launchSource.getStringExtra(WALLPAPER_LAUNCH_SOURCE)) {
                case LAUNCH_SOURCE_LAUNCHER:
                    return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_LAUNCHER;
                case LAUNCH_SOURCE_SETTINGS:
                    return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_SETTINGS;
                case LAUNCH_SOURCE_SUW:
                    return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_SUW;
                case LAUNCH_SOURCE_TIPS:
                    return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_TIPS;
                case LAUNCH_SOURCE_DEEP_LINK:
                    return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_DEEP_LINK;
                default:
                    return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_PREFERENCE_UNSPECIFIED;
            }
        } else if (launchSource.hasExtra(LAUNCH_SETTINGS_SEARCH)) {
            return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_SETTINGS_SEARCH;
        } else if (launchSource.getAction() != null && launchSource.getAction().equals(
                WallpaperManager.ACTION_CROP_AND_SET_WALLPAPER)) {
            return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_CROP_AND_SET_ACTION;
        } else if (launchSource.getCategories() != null
                && launchSource.getCategories().contains(Intent.CATEGORY_LAUNCHER)) {
            return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_LAUNCH_ICON;
        } else {
            return STYLE_UICHANGED__LAUNCHED_PREFERENCE__LAUNCHED_PREFERENCE_UNSPECIFIED;
        }
    }

    private int getCollectionIdHashCode(String collectionId) {
        return collectionId != null ? collectionId.hashCode() : 0;
    }
}
