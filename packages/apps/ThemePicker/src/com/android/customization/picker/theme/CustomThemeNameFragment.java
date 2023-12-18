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
package com.android.customization.picker.theme;

import android.os.Bundle;
import android.text.TextUtils;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.ImageView;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import com.android.customization.model.theme.ThemeBundle.PreviewInfo;
import com.android.customization.model.theme.custom.CustomTheme;
import com.android.customization.module.CustomizationInjector;
import com.android.customization.module.CustomizationPreferences;
import com.android.customization.picker.WallpaperPreviewer;
import com.android.wallpaper.R;
import com.android.wallpaper.module.CurrentWallpaperInfoFactory;
import com.android.wallpaper.module.InjectorProvider;
import com.android.wallpaper.picker.AppbarFragment;

import org.json.JSONArray;
import org.json.JSONException;

/** Fragment of naming a custom theme. */
public class CustomThemeNameFragment extends CustomThemeStepFragment {

    private static final String TAG = "CustomThemeNameFragment";

    public static CustomThemeNameFragment newInstance(CharSequence toolbarTitle, int position,
            int titleResId, int accessibilityResId) {
        CustomThemeNameFragment fragment = new CustomThemeNameFragment();
        Bundle arguments = AppbarFragment.createArguments(toolbarTitle);
        arguments.putInt(ARG_KEY_POSITION, position);
        arguments.putInt(ARG_KEY_TITLE_RES_ID, titleResId);
        arguments.putInt(ARG_KEY_ACCESSIBILITY_RES_ID, accessibilityResId);
        fragment.setArguments(arguments);
        return fragment;
    }

    private EditText mNameEditor;
    private ImageView mWallpaperImage;
    private ThemeOptionPreviewer mThemeOptionPreviewer;
    private CustomizationPreferences mCustomizationPreferences;

    @Nullable
    @Override
    public View onCreateView(@NonNull LayoutInflater inflater, @Nullable ViewGroup container,
            @Nullable Bundle savedInstanceState) {
        View view = super.onCreateView(inflater, container, savedInstanceState);
        mTitle = view.findViewById(R.id.component_options_title);
        mTitle.setText(mTitleResId);
        CurrentWallpaperInfoFactory currentWallpaperFactory = InjectorProvider.getInjector()
                .getCurrentWallpaperFactory(getActivity().getApplicationContext());
        CustomizationInjector injector = (CustomizationInjector) InjectorProvider.getInjector();
        mCustomizationPreferences = injector.getCustomizationPreferences(getContext());

        // Set theme option.
        ViewGroup previewContainer = view.findViewById(R.id.theme_preview_container);
        mThemeOptionPreviewer = new ThemeOptionPreviewer(getLifecycle(), getContext(),
                previewContainer);
        PreviewInfo previewInfo = mCustomThemeManager.buildCustomThemePreviewInfo(getContext());
        mThemeOptionPreviewer.setPreviewInfo(previewInfo);

        // Set wallpaper background.
        mWallpaperImage = view.findViewById(R.id.wallpaper_preview_image);
        final WallpaperPreviewer wallpaperPreviewer = new WallpaperPreviewer(
                getLifecycle(),
                getActivity(),
                mWallpaperImage,
                view.findViewById(R.id.wallpaper_preview_surface));
        currentWallpaperFactory.createCurrentWallpaperInfos(
                (homeWallpaper, lockWallpaper, presentationMode) -> {
                    wallpaperPreviewer.setWallpaper(homeWallpaper,
                            mThemeOptionPreviewer::updateColorForLauncherWidgets);
                }, false);

        // Set theme default name.
        mNameEditor = view.findViewById(R.id.custom_theme_name);
        mNameEditor.setText(getOriginalThemeName());
        return view;
    }

    private String getOriginalThemeName() {
        CustomTheme originalTheme = mCustomThemeManager.getOriginalTheme();
        if (originalTheme == null || !originalTheme.isDefined()) {
            // For new custom theme. use custom themes amount plus 1 as default naming.
            String serializedThemes = mCustomizationPreferences.getSerializedCustomThemes();
            int customThemesCount = 0;
            if (!TextUtils.isEmpty(serializedThemes)) {
                try {
                    JSONArray customThemes = new JSONArray(serializedThemes);
                    customThemesCount = customThemes.length();
                } catch (JSONException e) {
                    Log.w(TAG, "Couldn't read stored custom theme");
                }
            }
            return getContext().getString(
                    R.string.custom_theme_title, customThemesCount + 1);
        } else {
            // For existing custom theme, keep its name as default naming.
            return originalTheme.getTitle();
        }
    }

    @Override
    protected int getFragmentLayoutResId() {
        return R.layout.fragment_custom_theme_name;
    }

    public String getThemeName() {
        return mNameEditor.getText().toString();
    }
}
