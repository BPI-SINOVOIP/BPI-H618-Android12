package com.android.settings.development;

import com.android.settingslib.core.AbstractPreferenceController;
import android.content.ContentResolver;
import android.content.Context;
import android.content.Intent;
import android.os.UserManager;
import android.os.SystemProperties;
import android.provider.Settings;
import android.support.v14.preference.SwitchPreference;
import android.support.v4.content.LocalBroadcastManager;
import android.support.v7.preference.Preference;
import android.support.v7.preference.PreferenceScreen;
import android.support.v7.preference.TwoStatePreference;
import android.text.TextUtils;

public class EnableUsb0DevicePreferenceController extends AbstractPreferenceController{
    private static final String KEY_ENABLE_USB0_DEVICE = "enable_usb0_device";
    private static final String PROPERTY_USB0_DEVICE = "persist.sys.usb0device";
    private SwitchPreference mPreference;

    public EnableUsb0DevicePreferenceController(Context context) {
        super(context);
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        if (isAvailable()) {
            mPreference = (SwitchPreference) screen.findPreference(KEY_ENABLE_USB0_DEVICE);
        }
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public String getPreferenceKey() {
        return KEY_ENABLE_USB0_DEVICE;
    }

    public boolean isUSB0DeviceEnabled() {
        return SystemProperties.getInt(PROPERTY_USB0_DEVICE,0) != 0;
    }

    @Override
    public void updateState(Preference preference) {
        mPreference.setChecked(isUSB0DeviceEnabled());
    }

    public void enablePreference(boolean enabled) {
        if (isAvailable()) {
            mPreference.setEnabled(enabled);
        }
    }

    public void resetPreference() {
        if (mPreference.isChecked()) {
            mPreference.setChecked(false);
            handlePreferenceTreeClick(mPreference);
        }
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (TextUtils.equals(KEY_ENABLE_USB0_DEVICE, preference.getKey())) {
            final SwitchPreference switchPreference = (SwitchPreference) preference;
            SystemProperties.set(PROPERTY_USB0_DEVICE,switchPreference.isChecked()? "1":"0");
            return true;
        } else {
            return false;
        }
    }

}
