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
package com.android.settings.display;

import android.content.Context;
import android.support.v14.preference.SwitchPreference;
import android.support.v7.preference.ListPreference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.Preference;

import com.android.settings.core.PreferenceControllerMixin;
import com.android.settings.widget.SeekBarPreference;
import com.android.settingslib.core.AbstractPreferenceController;

import java.io.File;

import android.os.CountDownTimer;
import android.app.Activity;
import android.app.AlertDialog;
import android.app.Dialog;
import android.app.Fragment;
import android.content.DialogInterface;
import android.content.DialogInterface.OnClickListener;
import com.softwinner.display.DisplaydClient;

public class DisplayOutputPreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    public static final int FORMAT_CHANGED_CONFIRM = 0xfb709394;

    private static final int FALLBACK_COUNTDOWN_INTERVAL_MS = 1000;
    private static final int FALLBACK_DISPLAY_MODE_TIMEOUT = 10;
    private static final String HDMI_STATE = "/sys/class/extcon/hdmi/state";
    private static final String KEY_OUTPUT_SETTING   = "output_setting";
    private static final String KEY_HDMI_OUTPUT_MODE = "hdmi_output_mode";

    private final Context mContext;
    private final DisplayOutputPreferenceHost mHost;
    private final DisplaydClient mProxy;
    private final DisplayOutputImpl mImpl;

    private CountDownTimer mCountdownTimer;
    private ListPreference mHdmiOutputMode;

    public DisplayOutputPreferenceController(Context context,
            DisplayOutputPreferenceHost host) {
        super(context);
        mProxy = new DisplaydClient(context);
        mImpl = new DisplayOutputImpl();
        mContext = context;
        mHost = host;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        mHdmiOutputMode = (ListPreference) screen.findPreference(KEY_HDMI_OUTPUT_MODE);
        mHdmiOutputMode.setOnPreferenceChangeListener(this);
        mImpl.addPreference(mHdmiOutputMode, DisplayOutputImpl.TYPE_HDMI);
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_OUTPUT_SETTING;
    }

    @Override
    public void updateState(Preference preference) {
        if (mHdmiOutputMode != null) {
            mHdmiOutputMode.setValue(mImpl.getCurrentValue(mHdmiOutputMode));
        }
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        if (mHdmiOutputMode == preference) {
            mImpl.onModeSwitch(preference, newValue);
        }
        mHost.showSwitchConfirmDialog(FORMAT_CHANGED_CONFIRM);
        return true;
    }

    public interface DisplayOutputPreferenceHost {
        void showSwitchConfirmDialog(int id);
    };

    public Dialog buildConfirmDialog(Activity activity) {
        OnClickListener listener = new OnClickListener() {
            @Override
            public void onClick(DialogInterface dialog, int btn) {
                if (btn == AlertDialog.BUTTON_POSITIVE) {
                    mImpl.confirmSwitch();
                } else if (btn == AlertDialog.BUTTON_NEGATIVE) {
                    mImpl.cancelSwitch();
                }
                dialog.dismiss();
                mCountdownTimer.cancel();
                mCountdownTimer = null;
            }
        };

        String warning =
                mContext.getString(com.android.settings.R.string.display_mode_time_out_desc);
        final AlertDialog dialog = new AlertDialog.Builder(activity)
                .setTitle(com.android.settings.R.string.display_mode_time_out_title)
                .setMessage(String.format(warning, Integer.toString(FALLBACK_DISPLAY_MODE_TIMEOUT)))
                .setPositiveButton(com.android.internal.R.string.ok, listener)
                .setNegativeButton(com.android.internal.R.string.cancel, listener)
                .create();
        dialog.show();

        mCountdownTimer = new CountDownTimer(
                FALLBACK_DISPLAY_MODE_TIMEOUT * 1000, FALLBACK_COUNTDOWN_INTERVAL_MS) {
            private AlertDialog mDialog = dialog;

            @Override
            public void onTick(long millisUntilFinished) {
                final int secondsCountdown = (int) (millisUntilFinished / 1000);
                mDialog.setMessage(String.format(warning,
                            Integer.toString(secondsCountdown)));
            }

            @Override
            public void onFinish() {
                mImpl.cancelSwitch();
                mDialog.dismiss();
            }
         }.start();

        return dialog;
    }
}
