/*
 * Copyright (C) 2016 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed under the
 * License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */
package com.android.settings.sound;

import android.content.Context;
import android.os.RemoteException;
import android.util.Log;
import android.media.AudioManager;
import android.media.AudioManagerEx;

import androidx.preference.SwitchPreference;
import androidx.preference.ListPreference;
import androidx.preference.PreferenceScreen;
import androidx.preference.Preference;
import androidx.preference.PreferenceCategory;

import com.android.settings.R;
import com.android.settings.core.BasePreferenceController;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.RestrictedLockUtils;
import com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;
import com.android.settingslib.RestrictedLockUtilsInternal;
import com.android.settingslib.core.AbstractPreferenceController;

import java.util.ArrayList;

public class AudioOutputModePreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String TAG = "AudioOutputModeContr";

    private static final String KEY_AUDIO_OUTPUT = "audio_output_mode";
    private static final int AUDIO_CODEC = 0;
    private static final int AUDIO_HDMI = 1;
    private static final int AUDIO_SPDIF = 2;
    private static final int AUDIO_USB = 3;

    private ArrayList<String> mChannels;
    private AudioManagerEx mAudioManagerEx;

    public AudioOutputModePreferenceController(Context context) {
        super(context);

	mAudioManagerEx = new AudioManagerEx(context);
        mChannels = mAudioManagerEx.getActiveAudioDevices(AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_AUDIO_OUTPUT;
    }

    @Override
    public void updateState(Preference preference) {

        final AudioOutputModeListPreference audioModeListPreference = 
		(AudioOutputModeListPreference) preference;
	int currentAudioMode = getAudioMode();
	Log.d(TAG, "updateState: current audio output mode is " + currentAudioMode);
	audioModeListPreference.setValue(String.valueOf(currentAudioMode));

	updateAudioModePreferenceDescription(audioModeListPreference,
		Long.parseLong(audioModeListPreference.getValue()));
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        try {
            int value = Integer.parseInt((String) newValue); 
            Log.d(TAG, "onPreferenceChange: set audio output mode is " + value);
            setAudioMode(value);
            updateAudioModePreferenceDescription((AudioOutputModeListPreference) preference, value);
        } catch (NumberFormatException e) {
            Log.e(TAG, "could not persist audio output mode setting", e);
        }
	
	return true;
    }

    public static CharSequence getAudioOutputModeDescription(
            long currentAudioMode, CharSequence[] entries, CharSequence[] values) {
        if (currentAudioMode < 0 || entries == null || values == null
                || values.length != entries.length) {
            return null;
        }

        for (int i = 0; i < values.length; i++) {
            long audioMode = Long.parseLong(values[i].toString());
            if (currentAudioMode == audioMode) {
                return entries[i];
            }
        }
        return null;
    }

    private void updateAudioModePreferenceDescription(AudioOutputModeListPreference preference, 
             long currentAudioMode) {
        final CharSequence[] entries = preference.getEntries();
        final CharSequence[] values = preference.getEntryValues();
        final String summary;
        if (preference.isDisabledByAdmin()) {
            summary = mContext.getString(com.android.settings.R.string.disabled_by_policy_title);
        } else {
            if (0 > currentAudioMode || 3 < currentAudioMode) {
                summary = preference.getContext().getResources().getStringArray(R.array.audio_output_mode_entries)[1];
            } else {
                final CharSequence audioModeDescription = getAudioOutputModeDescription(
                        currentAudioMode, entries, values);
                if (audioModeDescription == null) {
                    summary = "";
                } else {
                    summary = mContext.getString(R.string.audio_output_mode_summary, audioModeDescription);
                }
            }
        }
        preference.setSummary(summary);
    }

    private void setAudioMode(int mode) {
	/*String st = null;
        for(int i = 0; i < mChannels.size(); i++){
            if(st == null){
                st = mChannels.get(i);
            }
            else{
                st = st + "," + mChannels.get(i);
            }
        }
        Log.d(TAG, "setAudioMode: mChannels is " + st);*/

	if (mChannels != null) {
            mChannels.clear();
	}

	if(mode == AUDIO_CODEC) {
            mChannels.add("AUDIO_CODEC");
	} else if (mode == AUDIO_HDMI) {
            mChannels.add("AUDIO_HDMI");
	} else if (mode == AUDIO_SPDIF) {
            mChannels.add("AUDIO_SPDIF");
	} else if (mode == AUDIO_SPDIF) {
	    mChannels.add("AUDIO_USB");
	}

	mAudioManagerEx.setAudioDeviceActive(mChannels, AudioManagerEx.AUDIO_OUTPUT_ACTIVE);
    }

    private int getAudioMode() {
	for (String chst: mChannels) {
            Log.d(TAG, "getAudioMode: mChannels is " + chst);
            if(chst.contains("CODEC")){
                return AUDIO_CODEC;
	    } else if(chst.contains("HDMI")){
                return AUDIO_HDMI;
	    } else if(chst.contains("SPDIF")){
                return AUDIO_SPDIF;
            } else if(chst.contains("USB")){
                return AUDIO_USB;
            }
	}

	return AUDIO_HDMI;
    }
}
