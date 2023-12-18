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

package com.android.tv.settings.device.displaysound;

import android.app.settings.SettingsEnums;
import android.content.ContentResolver;
import android.content.Context;
import android.os.Bundle;
import android.provider.Settings;

import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;

import com.android.tv.settings.R;
import com.android.tv.settings.RadioPreference;
import com.android.tv.settings.SettingsPreferenceFragment;

/**
 * The "Text scaling" screen in TV Settings.
 */
@Keep
public class FontScalePreferenceFragment extends SettingsPreferenceFragment implements
                Preference.OnPreferenceChangeListener {
    private static final String FONT_SCALE_RADIO_GROUP = "font_scale_radio_group";
    private static final String FONT_SCALE_GROUP = "font_scale_group";

    /** Value of FONT_SCALE. */
    private float mCurrentFontScaleValue;

    @Override
    public void onCreatePreferences(Bundle bundle, String s) {
        setPreferencesFromResource(R.xml.font_scale, null);
        PreferenceGroup fontScaleGroup = (PreferenceGroup) findPreference(FONT_SCALE_GROUP);
        final Context themedContext = getPreferenceManager().getContext();
        final String[] entryValues = getContext().getResources()
                .getStringArray(R.array.font_scale_entry_values);
        initFontScaleValue(getContext());

        for (int i = 0; i < entryValues.length; i++) {
            final RadioPreference preference = new RadioPreference(themedContext);
            preference.setPersistent(false);
            preference.setRadioGroup(FONT_SCALE_RADIO_GROUP);
            preference.setOnPreferenceChangeListener(this);
            preference.setKey(entryValues[i]);
            int scaleValue = (int) (Float.valueOf(entryValues[i]) * 100);
            String summary = getContext().getResources()
                    .getString(R.string.font_scale_item_detail, scaleValue);
            preference.setSummaryOff(summary);
            preference.setSummaryOn(summary);
            preference.setTitle(mapScaleIndexToTitle(i));
            preference.setFragment(FontScalePreviewFragment.class.getName());
            Bundle extras = preference.getExtras();
            extras.putString(FontScalePreviewFragment.PREVIEW_FONT_SCALE_VALUE, entryValues[i]);
            extras.putFloat(
                    FontScalePreviewFragment.CURRENT_FONT_SCALE_VALUE, mCurrentFontScaleValue);

            if (entryValues[i].equals(String.valueOf(mCurrentFontScaleValue))) {
                preference.setChecked(true);
            }
            fontScaleGroup.addPreference(preference);
        }
    }

    // Temporary method to accommodate additional font scale settings.
    // TODO: remove once font scale values are finalized.
    private String mapScaleIndexToTitle(int scaleValueIndex) {
        final String[] entries = getContext().getResources()
                .getStringArray(R.array.font_scale_entries);
        if (scaleValueIndex < 2) {
            return entries[scaleValueIndex];
        }
        if (scaleValueIndex > 3) {
            return entries[3];
        } else {
            return entries[2];
        }
    }

    private void initFontScaleValue(Context context) {
        final ContentResolver resolver = getContext().getContentResolver();
        mCurrentFontScaleValue =
                Settings.System.getFloat(resolver, Settings.System.FONT_SCALE, 1.0f);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        RadioPreference radioPreference = (RadioPreference) preference;
        if (radioPreference.isChecked()) {
            return false;
        }
        PreferenceGroup fontScaleGroup = (PreferenceGroup) findPreference(FONT_SCALE_GROUP);
        radioPreference.clearOtherRadioPreferences(fontScaleGroup);
        mCurrentFontScaleValue = Float.parseFloat(preference.getKey());
        commit();
        return true;
    }

    protected void commit() {
        if (getContext() == null) return;
        final ContentResolver resolver = getContext().getContentResolver();
        Settings.System.putFloat(resolver, Settings.System.FONT_SCALE, mCurrentFontScaleValue);
    }

    @Override
    protected int getPageId() {
        return SettingsEnums.ACCESSIBILITY_FONT_SIZE;
    }
}
