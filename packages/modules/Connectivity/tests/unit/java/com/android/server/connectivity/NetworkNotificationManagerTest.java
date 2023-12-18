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
 * limitations under the License.
 */

package com.android.server.connectivity;

import static android.app.Notification.FLAG_AUTO_CANCEL;
import static android.app.Notification.FLAG_ONGOING_EVENT;

import static com.android.server.connectivity.NetworkNotificationManager.NotificationType.LOST_INTERNET;
import static com.android.server.connectivity.NetworkNotificationManager.NotificationType.NETWORK_SWITCH;
import static com.android.server.connectivity.NetworkNotificationManager.NotificationType.NO_INTERNET;
import static com.android.server.connectivity.NetworkNotificationManager.NotificationType.PARTIAL_CONNECTIVITY;
import static com.android.server.connectivity.NetworkNotificationManager.NotificationType.PRIVATE_DNS_BROKEN;
import static com.android.server.connectivity.NetworkNotificationManager.NotificationType.SIGN_IN;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.clearInvocations;
import static org.mockito.Mockito.doReturn;
import static org.mockito.Mockito.eq;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.reset;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.app.Activity;
import android.app.Instrumentation;
import android.app.KeyguardManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageManager;
import android.content.res.Resources;
import android.net.ConnectivityResources;
import android.net.NetworkCapabilities;
import android.net.NetworkInfo;
import android.os.Build;
import android.os.Bundle;
import android.os.UserHandle;
import android.telephony.TelephonyManager;
import android.util.DisplayMetrics;
import android.widget.TextView;

import androidx.annotation.Nullable;
import androidx.annotation.StringRes;
import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;
import androidx.test.uiautomator.UiDevice;
import androidx.test.uiautomator.UiObject;
import androidx.test.uiautomator.UiSelector;

import com.android.connectivity.resources.R;
import com.android.server.connectivity.NetworkNotificationManager.NotificationType;
import com.android.testutils.DevSdkIgnoreRule;
import com.android.testutils.DevSdkIgnoreRunner;

import org.junit.After;
import org.junit.Before;
import org.junit.Ignore;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.AdditionalAnswers;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

@RunWith(DevSdkIgnoreRunner.class)
@SmallTest
@DevSdkIgnoreRule.IgnoreUpTo(Build.VERSION_CODES.R)
public class NetworkNotificationManagerTest {

    private static final String TEST_SSID = "Test SSID";
    private static final String TEST_EXTRA_INFO = "extra";
    private static final int TEST_NOTIF_ID = 101;
    private static final String TEST_NOTIF_TAG = NetworkNotificationManager.tagFor(TEST_NOTIF_ID);
    private static final long TEST_TIMEOUT_MS = 10_000L;
    static final NetworkCapabilities CELL_CAPABILITIES = new NetworkCapabilities();
    static final NetworkCapabilities WIFI_CAPABILITIES = new NetworkCapabilities();
    static final NetworkCapabilities VPN_CAPABILITIES = new NetworkCapabilities();
    static {
        CELL_CAPABILITIES.addTransportType(NetworkCapabilities.TRANSPORT_CELLULAR);
        CELL_CAPABILITIES.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);

        WIFI_CAPABILITIES.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        WIFI_CAPABILITIES.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        WIFI_CAPABILITIES.setSSID(TEST_SSID);

        // Set the underyling network to wifi.
        VPN_CAPABILITIES.addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        VPN_CAPABILITIES.addTransportType(NetworkCapabilities.TRANSPORT_VPN);
        VPN_CAPABILITIES.addCapability(NetworkCapabilities.NET_CAPABILITY_INTERNET);
        VPN_CAPABILITIES.removeCapability(NetworkCapabilities.NET_CAPABILITY_NOT_VPN);
    }

    /**
     * Test activity that shows the action it was started with on screen, and dismisses when the
     * text is tapped.
     */
    public static class TestDialogActivity extends Activity {
        @Override
        protected void onCreate(@Nullable Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);
            setTurnScreenOn(true);
            getSystemService(KeyguardManager.class).requestDismissKeyguard(
                    this, null /* callback */);

            final TextView txt = new TextView(this);
            txt.setText(getIntent().getAction());
            txt.setOnClickListener(e -> finish());
            setContentView(txt);
        }
    }

    @Mock Context mCtx;
    @Mock Resources mResources;
    @Mock DisplayMetrics mDisplayMetrics;
    @Mock PackageManager mPm;
    @Mock TelephonyManager mTelephonyManager;
    @Mock NotificationManager mNotificationManager;
    @Mock NetworkAgentInfo mWifiNai;
    @Mock NetworkAgentInfo mCellNai;
    @Mock NetworkAgentInfo mVpnNai;
    @Mock NetworkInfo mNetworkInfo;
    ArgumentCaptor<Notification> mCaptor;

    NetworkNotificationManager mManager;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        mCaptor = ArgumentCaptor.forClass(Notification.class);
        mWifiNai.networkCapabilities = WIFI_CAPABILITIES;
        mWifiNai.networkInfo = mNetworkInfo;
        mCellNai.networkCapabilities = CELL_CAPABILITIES;
        mCellNai.networkInfo = mNetworkInfo;
        mVpnNai.networkCapabilities = VPN_CAPABILITIES;
        mVpnNai.networkInfo = mNetworkInfo;
        mDisplayMetrics.density = 2.275f;
        doReturn(true).when(mVpnNai).isVPN();
        when(mCtx.getResources()).thenReturn(mResources);
        when(mCtx.getPackageManager()).thenReturn(mPm);
        when(mCtx.getApplicationInfo()).thenReturn(new ApplicationInfo());
        final Context asUserCtx = mock(Context.class, AdditionalAnswers.delegatesTo(mCtx));
        doReturn(UserHandle.ALL).when(asUserCtx).getUser();
        when(mCtx.createContextAsUser(eq(UserHandle.ALL), anyInt())).thenReturn(asUserCtx);
        when(mCtx.getSystemService(eq(Context.NOTIFICATION_SERVICE)))
                .thenReturn(mNotificationManager);
        when(mNetworkInfo.getExtraInfo()).thenReturn(TEST_EXTRA_INFO);
        ConnectivityResources.setResourcesContextForTest(mCtx);
        when(mResources.getColor(anyInt(), any())).thenReturn(0xFF607D8B);
        when(mResources.getDisplayMetrics()).thenReturn(mDisplayMetrics);

        // Come up with some credible-looking transport names. The actual values do not matter.
        String[] transportNames = new String[NetworkCapabilities.MAX_TRANSPORT + 1];
        for (int transport = 0; transport <= NetworkCapabilities.MAX_TRANSPORT; transport++) {
            transportNames[transport] = NetworkCapabilities.transportNameOf(transport);
        }
        when(mResources.getStringArray(R.array.network_switch_type_name))
            .thenReturn(transportNames);
        when(mResources.getBoolean(R.bool.config_autoCancelNetworkNotifications)).thenReturn(true);

        mManager = new NetworkNotificationManager(mCtx, mTelephonyManager);
    }

    @After
    public void tearDown() {
        ConnectivityResources.setResourcesContextForTest(null);
    }

    private void verifyTitleByNetwork(final int id, final NetworkAgentInfo nai, final int title) {
        final String tag = NetworkNotificationManager.tagFor(id);
        mManager.showNotification(id, PRIVATE_DNS_BROKEN, nai, null, null, true);
        verify(mNotificationManager, times(1))
                .notify(eq(tag), eq(PRIVATE_DNS_BROKEN.eventId), any());
        final int transportType = NetworkNotificationManager.approximateTransportType(nai);
        if (transportType == NetworkCapabilities.TRANSPORT_WIFI) {
            verify(mResources, times(1)).getString(eq(title), eq(TEST_EXTRA_INFO));
        } else {
            verify(mResources, times(1)).getString(title);
        }
        verify(mResources, times(1)).getString(eq(R.string.private_dns_broken_detailed));
    }

    @Test
    public void testTitleOfPrivateDnsBroken() {
        // Test the title of mobile data.
        verifyTitleByNetwork(100, mCellNai, R.string.mobile_no_internet);
        clearInvocations(mResources);

        // Test the title of wifi.
        verifyTitleByNetwork(101, mWifiNai, R.string.wifi_no_internet);
        clearInvocations(mResources);

        // Test the title of other networks.
        verifyTitleByNetwork(102, mVpnNai, R.string.other_networks_no_internet);
        clearInvocations(mResources);
    }

    @Test
    public void testNotificationsShownAndCleared() {
        final int NETWORK_ID_BASE = 100;
        List<NotificationType> types = Arrays.asList(NotificationType.values());
        List<Integer> ids = new ArrayList<>(types.size());
        for (int i = 0; i < types.size(); i++) {
            ids.add(NETWORK_ID_BASE + i);
        }
        Collections.shuffle(ids);
        Collections.shuffle(types);

        for (int i = 0; i < ids.size(); i++) {
            mManager.showNotification(ids.get(i), types.get(i), mWifiNai, mCellNai, null, false);
        }

        List<Integer> idsToClear = new ArrayList<>(ids);
        Collections.shuffle(idsToClear);
        for (int i = 0; i < ids.size(); i++) {
            mManager.clearNotification(idsToClear.get(i));
        }

        for (int i = 0; i < ids.size(); i++) {
            final int id = ids.get(i);
            final int eventId = types.get(i).eventId;
            final String tag = NetworkNotificationManager.tagFor(id);
            verify(mNotificationManager, times(1)).notify(eq(tag), eq(eventId), any());
            verify(mNotificationManager, times(1)).cancel(eq(tag), eq(eventId));
        }
    }

    @Test
    @Ignore
    // Ignored because the code under test calls Log.wtf, which crashes the tests on eng builds.
    // TODO: re-enable after fixing this (e.g., turn Log.wtf into exceptions that this test catches)
    public void testNoInternetNotificationsNotShownForCellular() {
        mManager.showNotification(100, NO_INTERNET, mCellNai, mWifiNai, null, false);
        mManager.showNotification(101, LOST_INTERNET, mCellNai, mWifiNai, null, false);

        verify(mNotificationManager, never()).notify(any(), anyInt(), any());

        mManager.showNotification(102, NO_INTERNET, mWifiNai, mCellNai, null, false);

        final int eventId = NO_INTERNET.eventId;
        final String tag = NetworkNotificationManager.tagFor(102);
        verify(mNotificationManager, times(1)).notify(eq(tag), eq(eventId), any());
    }

    @Test
    public void testNotificationsNotShownIfNoInternetCapability() {
        mWifiNai.networkCapabilities = new NetworkCapabilities();
        mWifiNai.networkCapabilities .addTransportType(NetworkCapabilities.TRANSPORT_WIFI);
        mManager.showNotification(102, NO_INTERNET, mWifiNai, mCellNai, null, false);
        mManager.showNotification(103, LOST_INTERNET, mWifiNai, mCellNai, null, false);
        mManager.showNotification(104, NETWORK_SWITCH, mWifiNai, mCellNai, null, false);

        verify(mNotificationManager, never()).notify(any(), anyInt(), any());
    }

    private void assertNotification(NotificationType type, boolean ongoing, boolean autoCancel) {
        final ArgumentCaptor<Notification> noteCaptor = ArgumentCaptor.forClass(Notification.class);
        mManager.showNotification(TEST_NOTIF_ID, type, mWifiNai, mCellNai, null, false);
        verify(mNotificationManager, times(1)).notify(eq(TEST_NOTIF_TAG), eq(type.eventId),
                noteCaptor.capture());

        assertEquals("Notification ongoing flag should be " + (ongoing ? "set" : "unset"),
                ongoing, (noteCaptor.getValue().flags & FLAG_ONGOING_EVENT) != 0);
        assertEquals("Notification autocancel flag should be " + (autoCancel ? "set" : "unset"),
                autoCancel, (noteCaptor.getValue().flags & FLAG_AUTO_CANCEL) != 0);
    }

    @Test
    public void testDuplicatedNotificationsNoInternetThenSignIn() {
        // Show first NO_INTERNET
        assertNotification(NO_INTERNET, false /* ongoing */, true /* autoCancel */);

        // Captive portal detection triggers SIGN_IN a bit later, clearing the previous NO_INTERNET
        assertNotification(SIGN_IN, false /* ongoing */, true /* autoCancel */);
        verify(mNotificationManager, times(1)).cancel(eq(TEST_NOTIF_TAG), eq(NO_INTERNET.eventId));

        // Network disconnects
        mManager.clearNotification(TEST_NOTIF_ID);
        verify(mNotificationManager, times(1)).cancel(eq(TEST_NOTIF_TAG), eq(SIGN_IN.eventId));
    }

    @Test
    public void testOngoingSignInNotification() {
        doReturn(true).when(mResources).getBoolean(R.bool.config_ongoingSignInNotification);

        // Show first NO_INTERNET
        assertNotification(NO_INTERNET, false /* ongoing */, true /* autoCancel */);

        // Captive portal detection triggers SIGN_IN a bit later, clearing the previous NO_INTERNET
        assertNotification(SIGN_IN, true /* ongoing */, true /* autoCancel */);
        verify(mNotificationManager, times(1)).cancel(eq(TEST_NOTIF_TAG), eq(NO_INTERNET.eventId));

        // Network disconnects
        mManager.clearNotification(TEST_NOTIF_ID);
        verify(mNotificationManager, times(1)).cancel(eq(TEST_NOTIF_TAG), eq(SIGN_IN.eventId));
    }

    @Test
    public void testNoAutoCancelNotification() {
        doReturn(false).when(mResources).getBoolean(R.bool.config_autoCancelNetworkNotifications);

        // Show NO_INTERNET, then SIGN_IN
        assertNotification(NO_INTERNET, false /* ongoing */, false /* autoCancel */);
        assertNotification(SIGN_IN, false /* ongoing */, false /* autoCancel */);
        verify(mNotificationManager, times(1)).cancel(eq(TEST_NOTIF_TAG), eq(NO_INTERNET.eventId));

        mManager.clearNotification(TEST_NOTIF_ID);
        verify(mNotificationManager, times(1)).cancel(eq(TEST_NOTIF_TAG), eq(SIGN_IN.eventId));
    }

    @Test
    public void testDuplicatedNotificationsSignInThenNoInternet() {
        final int id = TEST_NOTIF_ID;
        final String tag = TEST_NOTIF_TAG;

        // Show first SIGN_IN
        mManager.showNotification(id, SIGN_IN, mWifiNai, mCellNai, null, false);
        verify(mNotificationManager, times(1)).notify(eq(tag), eq(SIGN_IN.eventId), any());
        reset(mNotificationManager);

        // NO_INTERNET arrives after, but is ignored.
        mManager.showNotification(id, NO_INTERNET, mWifiNai, mCellNai, null, false);
        verify(mNotificationManager, never()).cancel(any(), anyInt());
        verify(mNotificationManager, never()).notify(any(), anyInt(), any());

        // Network disconnects
        mManager.clearNotification(id);
        verify(mNotificationManager, times(1)).cancel(eq(tag), eq(SIGN_IN.eventId));
    }

    @Test
    public void testClearNotificationByType() {
        final int id = TEST_NOTIF_ID;
        final String tag = TEST_NOTIF_TAG;

        // clearNotification(int id, NotificationType notifyType) will check if given type is equal
        // to previous type or not. If they are equal then clear the notification; if they are not
        // equal then return.
        mManager.showNotification(id, NO_INTERNET, mWifiNai, mCellNai, null, false);
        verify(mNotificationManager, times(1)).notify(eq(tag), eq(NO_INTERNET.eventId), any());

        // Previous notification is NO_INTERNET and given type is NO_INTERNET too. The notification
        // should be cleared.
        mManager.clearNotification(id, NO_INTERNET);
        verify(mNotificationManager, times(1)).cancel(eq(tag), eq(NO_INTERNET.eventId));

        // SIGN_IN is popped-up.
        mManager.showNotification(id, SIGN_IN, mWifiNai, mCellNai, null, false);
        verify(mNotificationManager, times(1)).notify(eq(tag), eq(SIGN_IN.eventId), any());

        // The notification type is not matching previous one, PARTIAL_CONNECTIVITY won't be
        // cleared.
        mManager.clearNotification(id, PARTIAL_CONNECTIVITY);
        verify(mNotificationManager, never()).cancel(eq(tag), eq(PARTIAL_CONNECTIVITY.eventId));
    }

    @Test
    public void testNotifyNoInternetAsDialogWhenHighPriority() throws Exception {
        doReturn(true).when(mResources).getBoolean(
                R.bool.config_notifyNoInternetAsDialogWhenHighPriority);

        mManager.showNotification(TEST_NOTIF_ID, NETWORK_SWITCH, mWifiNai, mCellNai, null, false);
        // Non-"no internet" notifications are not affected
        verify(mNotificationManager).notify(eq(TEST_NOTIF_TAG), eq(NETWORK_SWITCH.eventId), any());

        final Instrumentation instr = InstrumentationRegistry.getInstrumentation();
        final Context ctx = instr.getContext();
        final String testAction = "com.android.connectivity.coverage.TEST_DIALOG";
        final Intent intent = new Intent(testAction)
                .addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
                .setClassName(ctx.getPackageName(), TestDialogActivity.class.getName());
        final PendingIntent pendingIntent = PendingIntent.getActivity(ctx, 0 /* requestCode */,
                intent, PendingIntent.FLAG_CANCEL_CURRENT | PendingIntent.FLAG_IMMUTABLE);

        mManager.showNotification(TEST_NOTIF_ID, NO_INTERNET, mWifiNai, null /* switchToNai */,
                pendingIntent, true /* highPriority */);

        // Previous notifications are still dismissed
        verify(mNotificationManager).cancel(TEST_NOTIF_TAG, NETWORK_SWITCH.eventId);

        // Verify that the activity is shown (the activity shows the action on screen)
        final UiObject actionText = UiDevice.getInstance(instr).findObject(
                new UiSelector().text(testAction));
        assertTrue("Activity not shown", actionText.waitForExists(TEST_TIMEOUT_MS));

        // Tapping the text should dismiss the dialog
        actionText.click();
        assertTrue("Activity not dismissed", actionText.waitUntilGone(TEST_TIMEOUT_MS));

        // Verify no NO_INTERNET notification was posted
        verify(mNotificationManager, never()).notify(any(), eq(NO_INTERNET.eventId), any());
    }

    private void doNotificationTextTest(NotificationType type, @StringRes int expectedTitleRes,
            String expectedTitleArg, @StringRes int expectedContentRes) {
        final String expectedTitle = "title " + expectedTitleArg;
        final String expectedContent = "expected content";
        doReturn(expectedTitle).when(mResources).getString(expectedTitleRes, expectedTitleArg);
        doReturn(expectedContent).when(mResources).getString(expectedContentRes);

        mManager.showNotification(TEST_NOTIF_ID, type, mWifiNai, mCellNai, null, false);
        final ArgumentCaptor<Notification> notifCap = ArgumentCaptor.forClass(Notification.class);

        verify(mNotificationManager).notify(eq(TEST_NOTIF_TAG), eq(type.eventId),
                notifCap.capture());
        final Notification notif = notifCap.getValue();

        assertEquals(expectedTitle, notif.extras.getString(Notification.EXTRA_TITLE));
        assertEquals(expectedContent, notif.extras.getString(Notification.EXTRA_TEXT));
    }

    @Test
    public void testNotificationText_NoInternet() {
        doNotificationTextTest(NO_INTERNET,
                R.string.wifi_no_internet, TEST_EXTRA_INFO,
                R.string.wifi_no_internet_detailed);
    }

    @Test
    public void testNotificationText_Partial() {
        doNotificationTextTest(PARTIAL_CONNECTIVITY,
                R.string.network_partial_connectivity, TEST_EXTRA_INFO,
                R.string.network_partial_connectivity_detailed);
    }

    @Test
    public void testNotificationText_PartialAsNoInternet() {
        doReturn(true).when(mResources).getBoolean(
                R.bool.config_partialConnectivityNotifiedAsNoInternet);
        doNotificationTextTest(PARTIAL_CONNECTIVITY,
                R.string.wifi_no_internet, TEST_EXTRA_INFO,
                R.string.wifi_no_internet_detailed);
    }
}
