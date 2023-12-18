package com.softwinner.settingssetup;

import android.content.BroadcastReceiver;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.provider.Settings;
import android.text.TextUtils;
import android.util.Log;

public class SettingsReceiver extends BroadcastReceiver {
    private final static String TAG = "SettingsSetup";

    @Override
    public void onReceive(Context context, Intent intent) {
        Log.i(TAG, "onReceive " + intent.getAction());
        if (!Intent.ACTION_PRE_BOOT_COMPLETED.equals(intent.getAction()))
            return;
        loadSettings(context, R.array.secure_list, "secure");
        loadSettings(context, R.array.system_list, "system");
        loadSettings(context, R.array.global_list, "global");
        setEnabled(context, false);
    }

    public void loadSettings(Context context, int resId, String setting) {
        String[] items = context.getResources().getStringArray(resId);
        if (items != null) {
            for (String item : items) {
                if (TextUtils.isEmpty(item.trim()))
                    continue;
                String[] tmp = item.trim().split("=");
                if (tmp != null && tmp.length == 2) {
                    String key = tmp[0];
                    String value = tmp[1];
                    Log.i(TAG, "Settings put " + setting + " " + key + " " + value);
                    if ("secure".equals(setting)) {
                        Settings.Secure.putString(context.getContentResolver(), key, value);
                    } else if ("system".equals(setting)) {
                        Settings.System.putString(context.getContentResolver(), key, value);
                    } else if ("global".equals(setting)) {
                        Settings.Global.putString(context.getContentResolver(), key, value);
                    }
                }
            }
        }
    }

    private void setEnabled(Context context, boolean enabled) {
        PackageManager pm = context.getPackageManager();
        ComponentName component = new ComponentName("com.softwinner.settingssetup", "com.softwinner.settingssetup.SettingsReceiver");
        int state = pm.getComponentEnabledSetting(component);
        boolean isEnabled = state == PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        if (isEnabled != enabled || state == PackageManager.COMPONENT_ENABLED_STATE_DEFAULT) {
            pm.setComponentEnabledSetting(component, enabled
                    ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                    : PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
        }
    }
}
