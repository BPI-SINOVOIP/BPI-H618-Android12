/*
 * Copyright 2020 The Android Open Source Project
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

package com.google.android.iwlan;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;

import android.content.Context;
import android.net.ConnectivityManager;
import android.telephony.AccessNetworkConstants;
import android.telephony.INetworkService;
import android.telephony.INetworkServiceCallback;
import android.telephony.NetworkRegistrationInfo;
import android.telephony.NetworkServiceCallback;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;

import com.google.android.iwlan.IwlanNetworkService.IwlanNetworkServiceProvider;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;

import java.util.Arrays;

public class IwlanNetworkServiceTest {
    private static final String TAG = IwlanNetworkServiceTest.class.getSimpleName();
    private static final int DEFAULT_SLOT_INDEX = 0;

    @Mock private Context mMockContext;
    @Mock private ConnectivityManager mMockConnectivityManager;
    @Mock private SubscriptionManager mMockSubscriptionManager;
    @Mock private SubscriptionInfo mMockSubscriptionInfo;
    @Mock private INetworkServiceCallback mCallback;
    MockitoSession mStaticMockSession;

    IwlanNetworkService mIwlanNetworkService;
    INetworkService mBinder;
    IwlanNetworkServiceProvider mIwlanNetworkServiceProvider;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mStaticMockSession =
                mockitoSession()
                        .mockStatic(IwlanHelper.class)
                        .mockStatic(SubscriptionManager.class)
                        .startMocking();

        when(mMockContext.getSystemService(eq(ConnectivityManager.class)))
                .thenReturn(mMockConnectivityManager);

        when(mMockContext.getSystemService(eq(SubscriptionManager.class)))
                .thenReturn(mMockSubscriptionManager);

        when(mMockSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(
                        eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(mMockSubscriptionInfo);

        lenient()
                .when(SubscriptionManager.from(eq(mMockContext)))
                .thenReturn(mMockSubscriptionManager);

        mIwlanNetworkService = new IwlanNetworkService();
        mIwlanNetworkService.setAppContext(mMockContext);
        mIwlanNetworkServiceProvider = null;

        mBinder = mIwlanNetworkService.mBinder;
        mBinder.createNetworkServiceProvider(DEFAULT_SLOT_INDEX);
        mBinder.registerForNetworkRegistrationInfoChanged(DEFAULT_SLOT_INDEX, mCallback);
    }

    @After
    public void cleanUp() throws Exception {
        mBinder.removeNetworkServiceProvider(DEFAULT_SLOT_INDEX);
        mStaticMockSession.finishMocking();
    }

    @Test
    public void testRequestNetworkRegistrationInfo() throws Exception {
        int domain = NetworkRegistrationInfo.DOMAIN_PS;
        boolean mIsSubActive = true;
        long startTime;

        // Wait for IwlanNetworkServiceProvider created and timeout is 1 second.
        startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < 1000) {
            mIwlanNetworkServiceProvider =
                    mIwlanNetworkService.getNetworkServiceProvider(DEFAULT_SLOT_INDEX);
            if (mIwlanNetworkServiceProvider != null) {
                break;
            }
        }

        assertTrue(mIwlanNetworkServiceProvider != null);

        // Set Wifi on and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkService.setNetworkConnected(true, IwlanNetworkService.Transport.WIFI);
        verify(mCallback, timeout(1000).times(1)).onNetworkStateChanged();

        // Set Sub active and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkServiceProvider.subscriptionChanged();
        verify(mCallback, timeout(1000).times(2)).onNetworkStateChanged();

        // Create expected NetworkRegistrationInfo
        NetworkRegistrationInfo.Builder expectedStateBuilder =
                generateStateBuilder(
                        domain, mIsSubActive, NetworkRegistrationInfo.REGISTRATION_STATE_HOME);

        mBinder.requestNetworkRegistrationInfo(0, domain, mCallback);

        verify(mCallback, timeout(1000).times(1))
                .onRequestNetworkRegistrationInfoComplete(
                        eq(NetworkServiceCallback.RESULT_SUCCESS),
                        eq(expectedStateBuilder.build()));

        IwlanNetworkService.setNetworkConnected(
                false, IwlanNetworkService.Transport.UNSPECIFIED_NETWORK);
    }

    @Test
    public void testNetworkRegistrationInfoForCellularAndCstDisabled() throws Exception {
        int domain = NetworkRegistrationInfo.DOMAIN_PS;
        boolean mIsSubActive = true;
        long startTime;

        // Wait for IwlanNetworkServiceProvider created and timeout is 1 second.
        startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < 1000) {
            mIwlanNetworkServiceProvider =
                    mIwlanNetworkService.getNetworkServiceProvider(DEFAULT_SLOT_INDEX);
            if (mIwlanNetworkServiceProvider != null) {
                break;
            }
        }

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(false);
        lenient()
                .when(IwlanHelper.isDefaultDataSlot(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);

        assertTrue(mIwlanNetworkServiceProvider != null);

        // Set Network on and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkService.setNetworkConnected(true, IwlanNetworkService.Transport.MOBILE);
        verify(mCallback, timeout(1000).times(1)).onNetworkStateChanged();

        // Set Sub active and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkServiceProvider.subscriptionChanged();
        verify(mCallback, timeout(1000).times(2)).onNetworkStateChanged();

        // Create expected NetworkRegistrationInfo
        NetworkRegistrationInfo.Builder expectedStateBuilder =
                generateStateBuilder(
                        domain,
                        mIsSubActive,
                        NetworkRegistrationInfo.REGISTRATION_STATE_NOT_REGISTERED_SEARCHING);

        mBinder.requestNetworkRegistrationInfo(0, domain, mCallback);

        verify(mCallback, timeout(1000).times(1))
                .onRequestNetworkRegistrationInfoComplete(
                        eq(NetworkServiceCallback.RESULT_SUCCESS),
                        eq(expectedStateBuilder.build()));

        IwlanNetworkService.setNetworkConnected(
                false, IwlanNetworkService.Transport.UNSPECIFIED_NETWORK);
    }

    @Test
    public void testNetworkRegistrationInfoForCellularAndCstEnabled() throws Exception {
        int domain = NetworkRegistrationInfo.DOMAIN_PS;
        boolean mIsSubActive = true;
        long startTime;

        // Wait for IwlanNetworkServiceProvider created and timeout is 1 second.
        startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < 1000) {
            mIwlanNetworkServiceProvider =
                    mIwlanNetworkService.getNetworkServiceProvider(DEFAULT_SLOT_INDEX);
            if (mIwlanNetworkServiceProvider != null) {
                break;
            }
        }

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);
        lenient()
                .when(IwlanHelper.isDefaultDataSlot(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(false);

        assertTrue(mIwlanNetworkServiceProvider != null);

        // Set Network on and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkService.setNetworkConnected(true, IwlanNetworkService.Transport.MOBILE);
        verify(mCallback, timeout(1000).times(1)).onNetworkStateChanged();

        // Set Sub active and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkServiceProvider.subscriptionChanged();
        verify(mCallback, timeout(1000).times(2)).onNetworkStateChanged();

        // Create expected NetworkRegistrationInfo
        NetworkRegistrationInfo.Builder expectedStateBuilder =
                generateStateBuilder(
                        domain, mIsSubActive, NetworkRegistrationInfo.REGISTRATION_STATE_HOME);

        mBinder.requestNetworkRegistrationInfo(0, domain, mCallback);

        verify(mCallback, timeout(1000).times(1))
                .onRequestNetworkRegistrationInfoComplete(
                        eq(NetworkServiceCallback.RESULT_SUCCESS),
                        eq(expectedStateBuilder.build()));

        IwlanNetworkService.setNetworkConnected(
                false, IwlanNetworkService.Transport.UNSPECIFIED_NETWORK);
    }

    @Test
    public void testNetworkRegistrationInfoForWiFiAndCstEnabled() throws Exception {
        int domain = NetworkRegistrationInfo.DOMAIN_PS;
        boolean mIsSubActive = true;
        long startTime;

        // Wait for IwlanNetworkServiceProvider created and timeout is 1 second.
        startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < 1000) {
            mIwlanNetworkServiceProvider =
                    mIwlanNetworkService.getNetworkServiceProvider(DEFAULT_SLOT_INDEX);
            if (mIwlanNetworkServiceProvider != null) {
                break;
            }
        }

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);
        lenient()
                .when(IwlanHelper.isDefaultDataSlot(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);

        assertTrue(mIwlanNetworkServiceProvider != null);

        // Set Network on and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkService.setNetworkConnected(true, IwlanNetworkService.Transport.WIFI);
        verify(mCallback, timeout(1000).times(1)).onNetworkStateChanged();

        // Set Sub active and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkServiceProvider.subscriptionChanged();
        verify(mCallback, timeout(1000).times(2)).onNetworkStateChanged();

        // Create expected NetworkRegistrationInfo
        NetworkRegistrationInfo.Builder expectedStateBuilder =
                generateStateBuilder(
                        domain, mIsSubActive, NetworkRegistrationInfo.REGISTRATION_STATE_HOME);

        mBinder.requestNetworkRegistrationInfo(0, domain, mCallback);

        verify(mCallback, timeout(1000).times(1))
                .onRequestNetworkRegistrationInfoComplete(
                        eq(NetworkServiceCallback.RESULT_SUCCESS),
                        eq(expectedStateBuilder.build()));

        IwlanNetworkService.setNetworkConnected(
                false, IwlanNetworkService.Transport.UNSPECIFIED_NETWORK);
    }

    @Test
    public void testNetworkRegistrationInfoForWiFiAndCstDisabled() throws Exception {
        int domain = NetworkRegistrationInfo.DOMAIN_PS;
        boolean mIsSubActive = true;
        long startTime;

        // Wait for IwlanNetworkServiceProvider created and timeout is 1 second.
        startTime = System.currentTimeMillis();
        while (System.currentTimeMillis() - startTime < 1000) {
            mIwlanNetworkServiceProvider =
                    mIwlanNetworkService.getNetworkServiceProvider(DEFAULT_SLOT_INDEX);
            if (mIwlanNetworkServiceProvider != null) {
                break;
            }
        }

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(false);
        lenient()
                .when(IwlanHelper.isDefaultDataSlot(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);

        assertTrue(mIwlanNetworkServiceProvider != null);

        // Set Network on and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkService.setNetworkConnected(true, IwlanNetworkService.Transport.WIFI);
        verify(mCallback, timeout(1000).times(1)).onNetworkStateChanged();

        // Set Sub active and verify mCallback should receive onNetworkStateChanged.
        mIwlanNetworkServiceProvider.subscriptionChanged();
        verify(mCallback, timeout(1000).times(2)).onNetworkStateChanged();

        // Create expected NetworkRegistrationInfo
        NetworkRegistrationInfo.Builder expectedStateBuilder =
                generateStateBuilder(
                        domain, mIsSubActive, NetworkRegistrationInfo.REGISTRATION_STATE_HOME);

        mBinder.requestNetworkRegistrationInfo(0, domain, mCallback);

        verify(mCallback, timeout(1000).times(1))
                .onRequestNetworkRegistrationInfoComplete(
                        eq(NetworkServiceCallback.RESULT_SUCCESS),
                        eq(expectedStateBuilder.build()));

        IwlanNetworkService.setNetworkConnected(
                false, IwlanNetworkService.Transport.UNSPECIFIED_NETWORK);
    }

    private NetworkRegistrationInfo.Builder generateStateBuilder(
            int domain, boolean isSubActive, int registrationState) {
        NetworkRegistrationInfo.Builder expectedStateBuilder =
                new NetworkRegistrationInfo.Builder();
        expectedStateBuilder
                .setAccessNetworkTechnology(TelephonyManager.NETWORK_TYPE_IWLAN)
                .setAvailableServices(Arrays.asList(NetworkRegistrationInfo.SERVICE_TYPE_DATA))
                .setTransportType(AccessNetworkConstants.TRANSPORT_TYPE_WLAN)
                .setEmergencyOnly(!isSubActive)
                .setDomain(domain)
                .setRegistrationState(registrationState);

        return expectedStateBuilder;
    }
}
