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

package com.android.tv.settings.device.displaysound;

import static android.provider.Settings.Secure.MINIMAL_POST_PROCESSING_ALLOWED;

import static com.android.tv.settings.util.InstrumentationUtils.logToggleInteracted;

import android.content.Context;
import android.app.tvsettings.TvSettingsEnums;
import android.os.Bundle;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.SwitchPreference;

import java.util.ArrayList;
import java.util.List;

import com.android.settingslib.core.AbstractPreferenceController;
import com.android.tv.settings.PreferenceControllerFragment;
import com.android.tv.settings.R;
import com.android.tv.settings.device.display.HdmiAdvancePreferenceController;
import com.android.tv.settings.device.display.HdmiPreferenceController;
import com.android.tv.settings.device.display.CvbsPreferenceController;
import com.android.tv.settings.device.display.ScreenOrientationController;

/**
 * The "Advanced display settings" screen in TV Settings.
 */
@Keep
public class AdvancedDisplayFragment extends PreferenceControllerFragment {
    private static final String KEY_GAME_MODE = "game_mode";

    public static AdvancedDisplayFragment newInstance() {
        return new AdvancedDisplayFragment();
    }

    @Override
    protected int getPreferenceScreenResId() {
        return R.xml.advanced_display;
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        super.onCreatePreferences(savedInstanceState, rootKey);
        SwitchPreference gameModePreference = findPreference(KEY_GAME_MODE);
        gameModePreference.setChecked(getGameModeStatus() == 1);
    }

    @Override
    protected List<AbstractPreferenceController> onCreatePreferenceControllers(Context context) {
        final List<AbstractPreferenceController> controllers = new ArrayList<>();
        controllers.add(new HdmiPreferenceController(context));
        controllers.add(new CvbsPreferenceController(context));
        controllers.add(new ScreenOrientationController(context));
        controllers.add(new HdmiAdvancePreferenceController(context));
        return controllers;
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (TextUtils.equals(preference.getKey(), KEY_GAME_MODE)) {
            logToggleInteracted(
                    TvSettingsEnums.DISPLAY_SOUND_ADVANCED_DISPLAY_GAME_MODE,
                    ((SwitchPreference) preference).isChecked());
            setGameModeStatus(((SwitchPreference) preference).isChecked() ? 1 : 0);
        }
        return super.onPreferenceTreeClick(preference);
    }

    private int getGameModeStatus() {
        return Settings.Secure.getInt(getActivity().getContentResolver(),
                MINIMAL_POST_PROCESSING_ALLOWED,
                1);
    }

    private void setGameModeStatus(int state) {
        Settings.Secure.putInt(getActivity().getContentResolver(), MINIMAL_POST_PROCESSING_ALLOWED,
                state);
    }

    @Override
    protected int getPageId() {
        return TvSettingsEnums.DISPLAY_SOUND_ADVANCED_DISPLAY;
    }
}
