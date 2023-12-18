package com.softwinner.awmanager;

import android.app.Application;
import android.content.ComponentName;
import android.content.pm.PackageManager;
import android.aw.BackgroundManager;

public class AwManagerApp extends Application {
    private boolean mEnable;

    @Override
    public void onCreate() {
        super.onCreate();
        mEnable = BackgroundManager.getService() != null;
        if (!mEnable) {
            setTileEnabled(false);
        }
    }

    private void setTileEnabled(boolean enabled) {
        PackageManager pm = getPackageManager();
        ComponentName component = new ComponentName("com.softwinner.awmanager", "com.softwinner.awmanager.AwManager");
        int state = pm.getComponentEnabledSetting(component);
        boolean isEnabled = state == PackageManager.COMPONENT_ENABLED_STATE_ENABLED;
        if (isEnabled != enabled || state == PackageManager.COMPONENT_ENABLED_STATE_DEFAULT) {
            pm.setComponentEnabledSetting(component, enabled
                    ? PackageManager.COMPONENT_ENABLED_STATE_ENABLED
                    : PackageManager.COMPONENT_ENABLED_STATE_DISABLED,
                    PackageManager.DONT_KILL_APP);
        }
    }

    public boolean enable() {
        return mEnable;
    }

}
