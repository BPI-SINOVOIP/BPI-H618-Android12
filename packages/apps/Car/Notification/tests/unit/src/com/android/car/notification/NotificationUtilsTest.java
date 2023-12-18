/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.car.notification;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.ArgumentMatchers.anyString;
import static org.mockito.Mockito.when;

import android.app.Notification;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.service.notification.StatusBarNotification;
import android.testing.TestableContext;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.HashMap;
import java.util.Map;

@RunWith(AndroidJUnit4.class)
public class NotificationUtilsTest {
    @Rule
    public final TestableContext mContext = new TestableContext(
            InstrumentationRegistry.getInstrumentation().getTargetContext());

    private AlertEntry mAlertEntry;

    @Mock
    private StatusBarNotification mStatusBarNotification;
    @Mock
    private PackageManager mPackageManager;

    @Before
    public void setup() {
        MockitoAnnotations.initMocks(this);
        when(mStatusBarNotification.getKey()).thenReturn("TEST_KEY");
        when(mStatusBarNotification.getPackageName()).thenReturn("TEST_PACKAGE_NAME");
        mAlertEntry = new AlertEntry(mStatusBarNotification);
        mContext.setMockPackageManager(mPackageManager);
    }

    @Test
    public void onIsSystemPrivilegedOrPlatformKey_isPlatformKey_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ true, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemPrivilegedOrPlatformKey(mContext, mAlertEntry))
                .isTrue();
    }

    @Test
    public void onIsSystemPrivilegedOrPlatformKey_isSystemPrivileged_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                true, /* isPrivilegedApp= */ true);

        assertThat(NotificationUtils.isSystemPrivilegedOrPlatformKey(mContext, mAlertEntry))
                .isTrue();
    }

    @Test
    public void onIsSystemPrivilegedOrPlatformKey_isSystemNotPrivileged_returnsFalse()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                true, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemPrivilegedOrPlatformKey(mContext, mAlertEntry))
                .isFalse();
    }

    @Test
    public void onIsSystemPrivilegedOrPlatformKey_isNeither_returnsFalse()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemPrivilegedOrPlatformKey(mContext, mAlertEntry))
                .isFalse();
    }

    @Test
    public void onIsSystemOrPlatformKey_isPlatformKey_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ true, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemOrPlatformKey(mContext, mAlertEntry))
                .isTrue();
    }

    @Test
    public void onIsSystemOrPlatformKey_isSystemPrivileged_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                true, /* isPrivilegedApp= */ true);

        assertThat(NotificationUtils.isSystemOrPlatformKey(mContext, mAlertEntry))
                .isTrue();
    }

    @Test
    public void onIsSystemOrPlatformKey_isSystemNotPrivileged_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                true, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemOrPlatformKey(mContext, mAlertEntry))
                .isTrue();
    }

    @Test
    public void onIsSystemOrPlatformKey_isNeither_returnsFalse()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemOrPlatformKey(mContext, mAlertEntry))
                .isFalse();
    }

    @Test
    public void onIsSystemApp_isPlatformKey_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                true, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemApp(mContext, mAlertEntry.getStatusBarNotification()))
                .isTrue();
    }

    @Test
    public void onIsSystemApp_isNotPlatformKey_returnsFalse()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSystemApp(mContext, mAlertEntry.getStatusBarNotification()))
                .isFalse();
    }

    @Test
    public void onIsSignedWithPlatformKey_isSignedWithPlatformKey_returnsTrue()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ true, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSignedWithPlatformKey(mContext,
                mAlertEntry.getStatusBarNotification()))
                .isTrue();
    }

    @Test
    public void onIsSignedWithPlatformKey_isNotSignedWithPlatformKey_returnsFalse()
            throws PackageManager.NameNotFoundException {
        setApplicationInfo(/* signedWithPlatformKey= */ false, /* isSystemApp= */
                false, /* isPrivilegedApp= */ false);

        assertThat(NotificationUtils.isSignedWithPlatformKey(mContext,
                mAlertEntry.getStatusBarNotification()))
                .isFalse();
    }

    @Test
    public void onGetNotificationViewType_notificationIsARecognizedType_returnsCorrectType() {
        Map<String, CarNotificationTypeItem> typeMap = new HashMap<>();
        typeMap.put(Notification.CATEGORY_CAR_EMERGENCY, CarNotificationTypeItem.EMERGENCY);
        typeMap.put(Notification.CATEGORY_NAVIGATION, CarNotificationTypeItem.NAVIGATION);
        typeMap.put(Notification.CATEGORY_CALL, CarNotificationTypeItem.CALL);
        typeMap.put(Notification.CATEGORY_CAR_WARNING, CarNotificationTypeItem.WARNING);
        typeMap.put(Notification.CATEGORY_CAR_INFORMATION, CarNotificationTypeItem.INFORMATION);
        typeMap.put(Notification.CATEGORY_MESSAGE, CarNotificationTypeItem.MESSAGE);

        typeMap.forEach((category, typeItem) -> {
            Notification notification = new Notification();
            notification.category = category;
            when(mStatusBarNotification.getNotification()).thenReturn(notification);
            AlertEntry alertEntry = new AlertEntry(mStatusBarNotification);
            assertThat(NotificationUtils.getNotificationViewType(alertEntry)).isEqualTo(typeItem);
        });
    }

    @Test
    public void onGetNotificationViewType_notificationHasBigTextAndSummaryText_returnsInbox() {
        Bundle extras = new Bundle();
        extras.putBoolean(Notification.EXTRA_BIG_TEXT, true);
        extras.putBoolean(Notification.EXTRA_SUMMARY_TEXT, true);

        Notification notification = new Notification();
        notification.extras = extras;
        when(mStatusBarNotification.getNotification()).thenReturn(notification);
        AlertEntry alertEntry = new AlertEntry(mStatusBarNotification);

        assertThat(NotificationUtils.getNotificationViewType(alertEntry)).isEqualTo(
                CarNotificationTypeItem.INBOX);
    }

    @Test
    public void onGetNotificationViewType_unrecognizedTypeWithoutBigTextOrSummary_returnsBasic() {
        Notification notification = new Notification();
        when(mStatusBarNotification.getNotification()).thenReturn(notification);
        AlertEntry alertEntry = new AlertEntry(mStatusBarNotification);

        assertThat(NotificationUtils.getNotificationViewType(alertEntry)).isEqualTo(
                CarNotificationTypeItem.BASIC);
    }

    private void setApplicationInfo(boolean signedWithPlatformKey, boolean isSystemApp,
            boolean isPrivilegedApp) throws PackageManager.NameNotFoundException {
        ApplicationInfo applicationInfo = new ApplicationInfo();

        if (signedWithPlatformKey) {
            applicationInfo.privateFlags = applicationInfo.privateFlags
                    | ApplicationInfo.PRIVATE_FLAG_SIGNED_WITH_PLATFORM_KEY;
        }

        if (isSystemApp) {
            applicationInfo.flags = applicationInfo.flags
                    | ApplicationInfo.FLAG_SYSTEM;
        }

        if (isPrivilegedApp) {
            applicationInfo.privateFlags = applicationInfo.privateFlags
                    | ApplicationInfo.PRIVATE_FLAG_PRIVILEGED;
        }

        PackageInfo packageInfo = new PackageInfo();
        packageInfo.applicationInfo = applicationInfo;
        when(mPackageManager.getPackageInfoAsUser(anyString(), anyInt(), anyInt())).thenReturn(
                packageInfo);
    }
}
