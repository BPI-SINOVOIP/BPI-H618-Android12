/*
 * Copyright (C) 2016 The Android Open Source Project
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
 * limitations under the License
 */

package com.android.tv.settings.system;

import android.content.Context;
import android.media.tv.TvInputInfo;
import android.media.tv.TvInputManager;
import android.os.Bundle;
import android.os.UserHandle;
import android.provider.Settings;
import android.provider.Settings.Global;
import android.text.TextUtils;
import android.util.Log;

import androidx.annotation.Keep;
import androidx.preference.Preference;
import androidx.preference.PreferenceGroup;
import androidx.preference.TwoStatePreference;

import com.android.internal.app.LocalePicker;
import com.android.internal.app.LocalePicker.LocaleInfo;
import com.android.internal.logging.nano.MetricsProto.MetricsEvent;
import com.android.tv.settings.R;
import com.android.tv.settings.SettingsPreferenceFragment;

import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Fragment to control TV input settings.
 */
@Keep
public class InputsFragment extends SettingsPreferenceFragment implements
        Preference.OnPreferenceChangeListener {

    private static final String KEY_CONNECTED_INPUTS = "connected_inputs";
    private static final String KEY_STANDBY_INPUTS = "standby_inputs";
    private static final String KEY_DISCONNECTED_INPUTS = "disconnected_inputs";
    private static final String KEY_HDMI_CONTROL = "hdmi_control";
    private static final String KEY_DEVICE_AUTO_OFF = "device_auto_off";
    private static final String KEY_TV_AUTO_ON = "tv_auto_on";
    private static final String KEY_TV_AUTO_SET_LANGUAGE = "tv_auto_set_language";
    private static final String KEY_TV_AUTO_INPUT_PASSTHROUGH = "tv_auto_input_passthrough";

    private PreferenceGroup mConnectedGroup;
    private PreferenceGroup mStandbyGroup;
    private PreferenceGroup mDisconnectedGroup;

    private TwoStatePreference mHdmiControlPref;
    private TwoStatePreference mDeviceAutoOffPref;
    private TwoStatePreference mTvAutoOnPref;
    private TwoStatePreference mTvAutoSetLanguage;
    private TwoStatePreference mTvAutoInputPassThrough;

    private TvInputManager mTvInputManager;
    private Map<String, String> mCustomLabels;
    private Set<String> mHiddenIds;

    public static InputsFragment newInstance() {
        return new InputsFragment();
    }

    @Override
    public void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        mTvInputManager = (TvInputManager) getContext().getSystemService(Context.TV_INPUT_SERVICE);
    }

    @Override
    public void onResume() {
        super.onResume();
        final Context context = getContext();
        mCustomLabels =
                TvInputInfo.TvInputSettings.getCustomLabels(context, UserHandle.USER_SYSTEM);
        mHiddenIds =
                TvInputInfo.TvInputSettings.getHiddenTvInputIds(context, UserHandle.USER_SYSTEM);
        refresh();
    }

    @Override
    public void onCreatePreferences(Bundle savedInstanceState, String rootKey) {
        setPreferencesFromResource(R.xml.inputs, null);

        mConnectedGroup = (PreferenceGroup) findPreference(KEY_CONNECTED_INPUTS);
        mStandbyGroup = (PreferenceGroup) findPreference(KEY_STANDBY_INPUTS);
        mDisconnectedGroup = (PreferenceGroup) findPreference(KEY_DISCONNECTED_INPUTS);

        mHdmiControlPref = (TwoStatePreference) findPreference(KEY_HDMI_CONTROL);
        mHdmiControlPref.setOnPreferenceChangeListener(this);
        mDeviceAutoOffPref = (TwoStatePreference) findPreference(KEY_DEVICE_AUTO_OFF);
        mTvAutoOnPref = (TwoStatePreference) findPreference(KEY_TV_AUTO_ON);
        mTvAutoSetLanguage = (TwoStatePreference) findPreference(KEY_TV_AUTO_SET_LANGUAGE);
        mTvAutoInputPassThrough = (TwoStatePreference) findPreference(KEY_TV_AUTO_INPUT_PASSTHROUGH);
    }

    private void refresh() {
        mHdmiControlPref.setChecked(readCecOption(Settings.Global.HDMI_CONTROL_ENABLED));
        setPreferncesEnabled(mHdmiControlPref.isChecked());
        mDeviceAutoOffPref.setChecked(readCecOption(
                Settings.Global.HDMI_CONTROL_AUTO_DEVICE_OFF_ENABLED));
        mTvAutoOnPref.setChecked(readCecOption(Settings.Global.HDMI_CONTROL_AUTO_WAKEUP_ENABLED));
        mTvAutoSetLanguage.setChecked(readCecOption(Settings.Global.HDMI_CONTROL_AUTO_SET_LANGUAGE));
        mTvAutoInputPassThrough.setChecked(readCecOption(Settings.Global.HDMI_CONTROL_AUTO_INPUT_PASSTHROUGH));

        for (TvInputInfo info : mTvInputManager.getTvInputList()) {
            if (info.getType() == TvInputInfo.TYPE_TUNER
                    || !TextUtils.isEmpty(info.getParentId())) {
                continue;
            }

            int state;
            try {
                state = mTvInputManager.getInputState(info.getId());
            } catch (IllegalArgumentException e) {
                // Input is gone while iterating. Ignore.
                continue;
            }

            InputPreference inputPref = (InputPreference) findPreference(makeInputPrefKey(info));
            if (inputPref == null) {
                inputPref = new InputPreference(getPreferenceManager().getContext());
            }
            inputPref.refresh(info);

            switch (state) {
                case TvInputManager.INPUT_STATE_CONNECTED:
                    mStandbyGroup.removePreference(inputPref);
                    mDisconnectedGroup.removePreference(inputPref);
                    mConnectedGroup.addPreference(inputPref);
                    break;
                case TvInputManager.INPUT_STATE_CONNECTED_STANDBY:
                    mConnectedGroup.removePreference(inputPref);
                    mDisconnectedGroup.removePreference(inputPref);
                    mStandbyGroup.addPreference(inputPref);
                    break;
                case TvInputManager.INPUT_STATE_DISCONNECTED:
                    mConnectedGroup.removePreference(inputPref);
                    mStandbyGroup.removePreference(inputPref);
                    mDisconnectedGroup.addPreference(inputPref);
                    break;
            }
        }
        final int connectedCount = mConnectedGroup.getPreferenceCount();
        mConnectedGroup.setTitle(getResources().getQuantityString(
                R.plurals.inputs_header_connected_input,
                connectedCount));
        mConnectedGroup.setVisible(connectedCount > 0);

        final int standbyCount = mStandbyGroup.getPreferenceCount();
        mStandbyGroup.setTitle(getResources().getQuantityString(
                R.plurals.inputs_header_standby_input,
                standbyCount));
        mStandbyGroup.setVisible(standbyCount > 0);

        final int disconnectedCount = mDisconnectedGroup.getPreferenceCount();
        mDisconnectedGroup.setTitle(getResources().getQuantityString(
                R.plurals.inputs_header_disconnected_input,
                disconnectedCount));
        mDisconnectedGroup.setVisible(disconnectedCount > 0);
    }

    @Override
    public boolean onPreferenceTreeClick(Preference preference) {
        final String key = preference.getKey();
        Log.d("TvSettings","AWAW onPreferenceTreeClick key: " + key);
        if (key == null) {
            return super.onPreferenceTreeClick(preference);
        }
        switch (key) {
            case KEY_HDMI_CONTROL:
                writeCecOption(Settings.Global.HDMI_CONTROL_ENABLED, mHdmiControlPref.isChecked());
                int hdmiControlEnabled = readCecOption(Settings.Global.HDMI_CONTROL_ENABLED) ? 1 : 0;
                Log.d("TvSettings","AWAW hdmiControlEnabled: " + hdmiControlEnabled);
                if(mHdmiControlPref.isChecked() == false) {
                    writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_DEVICE_OFF_ENABLED,false);
                    writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_WAKEUP_ENABLED,false);
                    writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_SET_LANGUAGE,false);
                    writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_INPUT_PASSTHROUGH,false);

                    mDeviceAutoOffPref.setChecked(false);
                    mTvAutoOnPref.setChecked(false);
                    mTvAutoSetLanguage.setChecked(false);
                    mTvAutoInputPassThrough.setChecked(false);
                }
                return true;
            case KEY_DEVICE_AUTO_OFF:
                writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_DEVICE_OFF_ENABLED,
                        mDeviceAutoOffPref.isChecked());
                return true;
            case KEY_TV_AUTO_ON:
                writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_WAKEUP_ENABLED,
                        mTvAutoOnPref.isChecked());
                return true;
            case KEY_TV_AUTO_SET_LANGUAGE:
                writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_SET_LANGUAGE, mTvAutoSetLanguage.isChecked());
 //               String lan = Settings.Global.getString(getContext().getContentResolver(), Settings.Global.HDMI_CONTROL_TV_LANGUAGE);
 //               if(lan == null)
                String lan = "len";
                Log.d("cec", "lan = " + lan);
                Locale currentLocale = getContext().getResources().getConfiguration().locale;
                Log.d("cec", "current = " + currentLocale.getISO3Language());
                if (currentLocale.getISO3Language().equals(lan) ||
                        (currentLocale.getISO3Language().equals("zho") && lan.equals("chi"))) {
                    // Do not switch language if the new language is the same as the current one.
                    // This helps avoid accidental country variant switching from en_US to en_AU
                    // due to the limitation of CEC. See the warning below.
                    return true;
                }
                final List<LocaleInfo> localeInfos = LocalePicker.getAllAssetLocales(getContext(), false);
                for(LocaleInfo localeInfo : localeInfos) {
                    if(localeInfo.getLocale().getISO3Language().equals(lan)) {
                        LocalePicker.updateLocale(localeInfo.getLocale());
                    }
                    else if (localeInfo.getLocale().getISO3Language().equals("zho") &&
                            lan.equals("chi")) {
                        LocalePicker.updateLocale(localeInfo.getLocale());
                    }
                }
                return true;
            case KEY_TV_AUTO_INPUT_PASSTHROUGH:
                writeCecOption(Settings.Global.HDMI_CONTROL_AUTO_INPUT_PASSTHROUGH,
                        mTvAutoInputPassThrough.isChecked());
                return true;
        }
        return super.onPreferenceTreeClick(preference);
    }

    private boolean readCecOption(String key) {
        return Settings.Global.getInt(getContext().getContentResolver(), key, 0) == 1;
    }

    private void writeCecOption(String key, boolean value) {
        Settings.Global.putInt(getContext().getContentResolver(), key, value ? 1 : 0);
    }

    private class InputPreference extends Preference {
        public InputPreference(Context context) {
            super(context);
        }

        public void refresh(TvInputInfo inputInfo) {
            setKey(makeInputPrefKey(inputInfo));

            setTitle(inputInfo.loadLabel(getContext()));

            String customLabel;
            if (mHiddenIds.contains(inputInfo.getId())) {
                customLabel = getString(R.string.inputs_hide);
            } else {
                customLabel = mCustomLabels.get(inputInfo.getId());
                if (TextUtils.isEmpty(customLabel)) {
                    customLabel = inputInfo.loadLabel(getContext()).toString();
                }
            }
            setSummary(customLabel);
            setFragment(InputOptionsFragment.class.getName());
            InputOptionsFragment.prepareArgs(getExtras(), inputInfo);
        }
    }

    public static String makeInputPrefKey(TvInputInfo inputInfo) {
        return "InputPref:" + inputInfo.getId();
    }

    public int getMetricsCategory() {
        return MetricsEvent.SETTINGS_TV_INPUTS_CATEGORY;
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
            boolean isChecked = (boolean)newValue;
            setPreferncesEnabled(isChecked);
            return true;
    }

    private void setPreferncesEnabled(boolean value){
        mDeviceAutoOffPref.setEnabled(value);
        mTvAutoOnPref.setEnabled(value);
        mTvAutoSetLanguage.setEnabled(value);
        mTvAutoInputPassThrough.setEnabled(value);
    }
}
