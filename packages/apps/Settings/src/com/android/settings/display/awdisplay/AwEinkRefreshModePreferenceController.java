/*
 * Add by Setting Aw Eink refresh mode
 *
 * @author humingming@allwinnertech.com
 */

package com.android.settings.display.awdisplay;

import android.content.Context;
import android.content.ContentResolver;
import android.os.SystemProperties;
import android.provider.Settings;
import android.view.WindowManager;
import android.util.Log;

import androidx.preference.Preference;
import androidx.preference.PreferenceScreen;

import com.android.settings.R;
import com.android.settings.core.PreferenceControllerMixin;
import com.android.settingslib.RestrictedLockUtils;
import com.android.settingslib.RestrictedLockUtils.EnforcedAdmin;
import com.android.settingslib.RestrictedLockUtilsInternal;
import com.android.settingslib.core.AbstractPreferenceController;

public class AwEinkRefreshModePreferenceController extends AbstractPreferenceController implements
        PreferenceControllerMixin, Preference.OnPreferenceChangeListener {

    private static final String TAG = "AwEinkRefreshModePrefContr";
    private static final String KEY_EINK_REFRESH_MODE = "eink_refresh_mode";

    private ContentResolver mContentResolver;

    public AwEinkRefreshModePreferenceController(Context context) {
        super(context);
        mContentResolver = context.getContentResolver();
    }

    @Override
    public boolean isAvailable() {
        return mContext.getResources().getBoolean(R.bool.config_use_eink_screen);
    }

    @Override
    public String getPreferenceKey() {
        return KEY_EINK_REFRESH_MODE;
    }

    @Override
    public void updateState(Preference preference) {
        final AwEinkRefreshModeListPreference einkRefreshModeListPreference = (AwEinkRefreshModeListPreference) preference;
        final String currentMode = Settings.System.getString(mContentResolver, Settings.System.EINK_REFRESHMODE);
        einkRefreshModeListPreference.setValue(currentMode);
        einkRefreshModeListPreference.setSummary(currentMode);
    }

    @Override
    public boolean onPreferenceChange(Preference preference, Object newValue) {
        try {
            Settings.System.putString(mContentResolver,
                    Settings.System.EINK_REFRESHMODE, (String)newValue);
            ((AwEinkRefreshModeListPreference) preference).setSummary((CharSequence) newValue);
        } catch (NumberFormatException e) {
            Log.e(TAG, "could not persist eink refresh mode setting", e);
        }
        return true;
    }
}
