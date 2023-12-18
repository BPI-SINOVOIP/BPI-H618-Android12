package com.android.systemui.statusbar.tv;

import android.service.notification.StatusBarNotification;

import com.android.systemui.statusbar.notification.NotificationEntryManager;

import javax.inject.Inject;

public class TvKeyguardEnvironmentImpl implements NotificationEntryManager.KeyguardEnvironment {
    @Inject
    public TvKeyguardEnvironmentImpl() {
    }

    @Override
    public boolean isDeviceProvisioned() {
        return false;
    }

    @Override
    public boolean isNotificationForCurrentProfiles(StatusBarNotification sbn) {
        return false;
    }
}
