/*
 * Copyright (C) 2015 The Android Open Source Project
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

import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_CLASSIC;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_TWO_PANEL;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_VENDOR;
import static com.android.tv.settings.overlay.FlavorUtils.FLAVOR_X;
import static com.android.tv.settings.util.InstrumentationUtils.logToggleInteracted;

import android.app.tvsettings.TvSettingsEnums;
import android.content.ContentResolver;
import android.content.Context;
import android.media.AudioManager;
import android.os.Bundle;
import android.provider.Settings;
import android.text.TextUtils;

import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.TwoStatePreference;

import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;
import com.android.tv.settings.device.displaysound.AudioChannelsSelect;
import com.android.tv.settings.overlay.FlavorUtils;
import com.android.tv.settings.util.SliceUtils;
import com.android.tv.twopanelsettings.slices.SlicePreference;

/**
 * The "Display & sound" screen in TV Settings.
 */
@Keep
public class DisplaySoundFragment extends SettingsPreferenceFragment {

    static final String KEY_SOUND_EFFECTS = "sound_effects";
    private static final String KEY_AUDIO_OUTPUT_MODE = "audio_output_mode";
    private static final String KEY_ENABLE_PASS_THROUGH = "enable_pass_through";
    private static final String KEY_CEC = "cec";

    private AudioManager mAudioManager;
    private Preference mAudioOutputPreference;
    private AudioChannelsSelect mAudioSelector;
    private TwoStatePreference mEnablePassThrough;

    public static DisplaySoundFragment newInstance() {
        return new DisplaySoundFragment();
    }

    public static boolean getSoundEffectsEnabled(ContentResolver contentResolver) {
        return Settings.System.getInt(contentResolver, Settings.System.SOUND_EFFECTS_ENABLED, 1)
                != 0;
    }

    @Override
    public void onAttach(Context context) {
        mAudioManager = context.getSystemService(AudioManager.class);
        super.onAttach(context);
        mAudioSelector = new AudioChannelsSelect(context);
    }

    private int getPreferenceScreenResId() {
        switch (FlavorUtils.getFlavor(getContext())) {
            case FLAVOR_CLASSIC:
            case FLAVOR_TWO_PANEL:
                return R.xml.display_sound;
            case FLAVOR_X:
            case FLAVOR_VENDOR:
                return R.xml.display_sound_x;
            default:
                return R.xml.display_sound;
        }
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(getPreferenceScreenResId(), null);

        final TwoStatePreference soundPref = findPreference(KEY_SOUND_EFFECTS);
        soundPref.setChecked(getSoundEffectsEnabled());
        mEnablePassThrough = (TwoStatePreference) findPreference(KEY_ENABLE_PASS_THROUGH);
        mAudioOutputPreference = findPreference(KEY_AUDIO_OUTPUT_MODE);
        mEnablePassThrough.setChecked(Settings.System.getInt(getActivity().getContentResolver(),Settings.System.ENABLE_PASS_THROUGH, 0) != 0);
        getPreferenceScreen().removePreference(soundPref);
        updateCecPreference();
    }

    @Override
    public void onResume() {
        super.onResume();
        // Update the subtitle of CEC setting when navigating back to this page.
        updateCecPreference();
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        if (preference == mEnablePassThrough) {
            Settings.System.putInt(getActivity().getContentResolver(), Settings.System.ENABLE_PASS_THROUGH,
            mEnablePassThrough.isChecked() ? 1 : 0);
            return true;
        }
        if (preference == mAudioOutputPreference) {
            mAudioSelector.showChannelsSelectDialog();
            return true;
        }

        if (TextUtils.equals(preference.getKey(), KEY_SOUND_EFFECTS)) {
            final TwoStatePreference soundPref = (TwoStatePreference) preference;
            logToggleInteracted(TvSettingsEnums.DISPLAY_SOUND_SYSTEM_SOUNDS, soundPref.isChecked());
            setSoundEffectsEnabled(soundPref.isChecked());
        }
        return super.onPreferenceTreeClick(preference);
    }

    private boolean getSoundEffectsEnabled() {
        return getSoundEffectsEnabled(getActivity().getContentResolver());
    }

    private void setSoundEffectsEnabled(boolean enabled) {
        if (enabled) {
            mAudioManager.loadSoundEffects();
        } else {
            mAudioManager.unloadSoundEffects();
        }
        Settings.System.putInt(getActivity().getContentResolver(),
                Settings.System.SOUND_EFFECTS_ENABLED, enabled ? 1 : 0);
    }

    private void updateCecPreference() {
        Preference cecPreference = findPreference(KEY_CEC);
        if (cecPreference instanceof SlicePreference
                && SliceUtils.isSliceProviderValid(
                        getContext(), ((SlicePreference) cecPreference).getUri())) {
            ContentResolver resolver = getContext().getContentResolver();
            // Note that default CEC is enabled. You'll find similar retrieval of property in
            // HdmiControlService.
            boolean cecEnabled =
                    Settings.Global.getInt(resolver, Settings.Global.HDMI_CONTROL_ENABLED, 1) != 0;
            cecPreference.setSummary(cecEnabled ? R.string.enabled : R.string.disabled);
            cecPreference.setVisible(true);
        } else {
            cecPreference.setVisible(false);
        }
    }

    @Override
    protected int getPageId() {
        return TvSettingsEnums.DISPLAY_SOUND;
    }
}
