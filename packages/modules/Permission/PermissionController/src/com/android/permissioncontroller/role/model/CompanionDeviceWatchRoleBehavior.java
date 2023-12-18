/*
 * Copyright (C) 2020 The Android Open Source Project
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

package com.android.permissioncontroller.role.model;

import android.app.NotificationManager;
import android.content.ComponentName;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.content.pm.ServiceInfo;
import android.service.notification.NotificationListenerService;
import android.util.Log;

import androidx.annotation.NonNull;

import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

/**
 * Class for behavior of the "watch" Companion device profile role.
 */
public class CompanionDeviceWatchRoleBehavior implements RoleBehavior {

    public static final String LOG_TAG = CompanionDeviceWatchRoleBehavior.class.getSimpleName();

    @Override
    public void grant(@NonNull Role role, @NonNull String packageName, @NonNull Context context) {
        List<ComponentName> notificationListenersForPackage =
                getNotificationListenersForPackage(packageName, context);
        setNotificationGrantState(context, notificationListenersForPackage, true);
    }

    private void setNotificationGrantState(@NonNull Context context,
            List<ComponentName> notificationListenersForPackage, boolean granted) {
        NotificationManager notificationManager =
                context.getSystemService(NotificationManager.class);
        int size = notificationListenersForPackage.size();
        for (int i = 0; i < size; i++) {
            ComponentName componentName = notificationListenersForPackage.get(i);
            notificationManager.setNotificationListenerAccessGranted(
                    componentName, granted, false);
        }
    }

    private List<ComponentName> getNotificationListenersForPackage(@NonNull String packageName,
            @NonNull Context context) {
        List<ResolveInfo> allListeners = context.getPackageManager().queryIntentServices(
                new Intent(NotificationListenerService.SERVICE_INTERFACE).setPackage(packageName),
                PackageManager.MATCH_DIRECT_BOOT_AWARE | PackageManager.MATCH_DIRECT_BOOT_UNAWARE);
        ArrayList<ComponentName> pkgListeners = new ArrayList<>();
        int size = allListeners.size();
        for (int i = 0; i < size; i++) {
            ResolveInfo service = allListeners.get(i);
            ServiceInfo serviceInfo = service.serviceInfo;
            if (Objects.equals(serviceInfo.permission,
                    android.Manifest.permission.BIND_NOTIFICATION_LISTENER_SERVICE)
                    && packageName.equals(serviceInfo.packageName)) {
                pkgListeners.add(new ComponentName(serviceInfo.packageName, serviceInfo.name));
            }
        }
        Log.d(LOG_TAG, "getNotificationListenersForPackage(" + packageName + "): " + pkgListeners);
        return pkgListeners;
    }

    @Override
    public void revoke(@NonNull Role role, @NonNull String packageName, @NonNull Context context) {
        NotificationManager notificationManager =
                context.getSystemService(NotificationManager.class);
        List<ComponentName> enabledNotificationListeners =
                notificationManager.getEnabledNotificationListeners();
        setNotificationGrantState(context, enabledNotificationListeners, false);
    }
}
