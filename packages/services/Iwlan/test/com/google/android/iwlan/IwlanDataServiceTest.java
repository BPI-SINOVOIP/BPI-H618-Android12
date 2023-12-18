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

import static android.net.NetworkCapabilities.TRANSPORT_CELLULAR;
import static android.net.NetworkCapabilities.TRANSPORT_WIFI;

import static com.android.dx.mockito.inline.extended.ExtendedMockito.mockitoSession;

import static org.junit.Assert.*;
import static org.mockito.Matchers.isNull;
import static org.mockito.Mockito.*;

import android.content.ContentResolver;
import android.content.Context;
import android.net.ConnectivityManager;
import android.net.LinkAddress;
import android.net.LinkProperties;
import android.net.Network;
import android.net.NetworkCapabilities;
import android.telephony.AccessNetworkConstants.AccessNetworkType;
import android.telephony.DataFailCause;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.data.ApnSetting;
import android.telephony.data.DataCallResponse;
import android.telephony.data.DataProfile;
import android.telephony.data.DataService;
import android.telephony.data.DataServiceCallback;
import android.telephony.data.IDataServiceCallback;
import android.telephony.ims.ImsManager;
import android.telephony.ims.ImsMmTelManager;

import com.google.android.iwlan.IwlanDataService.IwlanDataServiceProvider;
import com.google.android.iwlan.IwlanDataService.IwlanDataServiceProvider.IwlanTunnelCallback;
import com.google.android.iwlan.IwlanDataService.IwlanDataServiceProvider.TunnelState;
import com.google.android.iwlan.IwlanDataService.IwlanNetworkMonitorCallback;
import com.google.android.iwlan.epdg.EpdgSelector;
import com.google.android.iwlan.epdg.EpdgTunnelManager;
import com.google.android.iwlan.epdg.TunnelLinkProperties;
import com.google.android.iwlan.epdg.TunnelLinkPropertiesTest;
import com.google.android.iwlan.epdg.TunnelSetupRequest;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;
import org.mockito.quality.Strictness;

import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.Date;
import java.util.List;
import java.util.LongSummaryStatistics;
import java.util.concurrent.CountDownLatch;
import java.util.concurrent.TimeUnit;

public class IwlanDataServiceTest {
    private static final int DEFAULT_SLOT_INDEX = 0;
    private static final int DEFAULT_SUB_INDEX = 0;
    private static final int LINK_MTU = 1280;
    private static final String TEST_APN_NAME = "ims";
    private static final String IP_ADDRESS = "192.0.2.1";
    private static final String DNS_ADDRESS = "8.8.8.8";
    private static final String GATEWAY_ADDRESS = "0.0.0.0";
    private static final String PSCF_ADDRESS = "10.159.204.230";
    private static final String INTERFACE_NAME = "ipsec6";

    @Mock private Context mMockContext;
    @Mock private SubscriptionManager mMockSubscriptionManager;
    @Mock private SubscriptionInfo mMockSubscriptionInfo;
    @Mock private ContentResolver mMockContentResolver;
    @Mock private ConnectivityManager mMockConnectivityManager;
    @Mock private DataServiceCallback mMockDataServiceCallback;
    @Mock private EpdgTunnelManager mMockEpdgTunnelManager;
    @Mock private IwlanDataServiceProvider mMockIwlanDataServiceProvider;
    @Mock private Network mMockNetwork;
    @Mock private NetworkCapabilities mMockNetworkCapabilities;
    @Mock private TunnelLinkProperties mMockTunnelLinkProperties;
    @Mock private ErrorPolicyManager mMockErrorPolicyManager;
    @Mock private ImsManager mMockImsManager;
    @Mock private ImsMmTelManager mMockImsMmTelManager;
    @Mock private TelephonyManager mMockTelephonyManager;
    @Mock private EpdgSelector mMockEpdgSelector;
    @Mock private LinkProperties mMockLinkProperties;
    @Mock private LinkAddress mMockIPv4LinkAddress;
    @Mock private LinkAddress mMockIPv6LinkAddress;
    @Mock private Inet4Address mMockInet4Address;
    @Mock private Inet6Address mMockInet6Address;
    MockitoSession mStaticMockSession;

    private List<DataCallResponse> mResultDataCallList;
    private @DataServiceCallback.ResultCode int mResultCode;
    private CountDownLatch latch;
    private IwlanDataService mIwlanDataService;
    private IwlanDataServiceProvider mIwlanDataServiceProvider;
    private IwlanDataServiceProvider mSpyIwlanDataServiceProvider;

    private final class IwlanDataServiceCallback extends IDataServiceCallback.Stub {

        private final String mTag;

        IwlanDataServiceCallback(String tag) {
            mTag = tag;
        }

        @Override
        public void onSetupDataCallComplete(
                @DataServiceCallback.ResultCode int resultCode, DataCallResponse response) {}

        @Override
        public void onDeactivateDataCallComplete(@DataServiceCallback.ResultCode int resultCode) {}

        @Override
        public void onSetInitialAttachApnComplete(@DataServiceCallback.ResultCode int resultCode) {}

        @Override
        public void onSetDataProfileComplete(@DataServiceCallback.ResultCode int resultCode) {}

        @Override
        public void onRequestDataCallListComplete(
                @DataServiceCallback.ResultCode int resultCode,
                List<DataCallResponse> dataCallList) {
            mResultCode = resultCode;
            mResultDataCallList = new ArrayList<DataCallResponse>(dataCallList);
            latch.countDown();
        }

        @Override
        public void onDataCallListChanged(List<DataCallResponse> dataCallList) {}

        @Override
        public void onHandoverStarted(@DataServiceCallback.ResultCode int result) {}

        @Override
        public void onHandoverCancelled(@DataServiceCallback.ResultCode int result) {}

        @Override
        public void onApnUnthrottled(String apn) {}
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        mStaticMockSession =
                mockitoSession()
                        .mockStatic(EpdgSelector.class)
                        .mockStatic(ErrorPolicyManager.class)
                        .mockStatic(IwlanBroadcastReceiver.class)
                        .mockStatic(IwlanHelper.class)
                        .strictness(Strictness.LENIENT)
                        .startMocking();

        when(mMockContext.getSystemService(eq(ConnectivityManager.class)))
                .thenReturn(mMockConnectivityManager);
        when(mMockConnectivityManager.getNetworkCapabilities(eq(mMockNetwork)))
                .thenReturn(mMockNetworkCapabilities);
        when(mMockNetworkCapabilities.hasTransport(eq(TRANSPORT_CELLULAR))).thenReturn(false);
        when(mMockNetworkCapabilities.hasTransport(eq(TRANSPORT_WIFI))).thenReturn(true);

        when(mMockContext.getSystemService(eq(SubscriptionManager.class)))
                .thenReturn(mMockSubscriptionManager);

        when(mMockSubscriptionManager.getActiveSubscriptionInfoForSimSlotIndex(anyInt()))
                .thenReturn(mMockSubscriptionInfo);

        when(mMockSubscriptionInfo.getSubscriptionId()).thenReturn(DEFAULT_SUB_INDEX);

        when(mMockContext.getSystemService(eq(TelephonyManager.class)))
                .thenReturn(mMockTelephonyManager);

        when(mMockTelephonyManager.createForSubscriptionId(eq(DEFAULT_SUB_INDEX)))
                .thenReturn(mMockTelephonyManager);

        when(mMockTelephonyManager.isNetworkRoaming()).thenReturn(false);

        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);

        when(mMockContext.getSystemService(eq(ImsManager.class))).thenReturn(mMockImsManager);

        when(mMockImsManager.getImsMmTelManager(anyInt())).thenReturn(mMockImsMmTelManager);

        when(mMockImsMmTelManager.isVoWiFiSettingEnabled()).thenReturn(false);

        when(EpdgSelector.getSelectorInstance(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(mMockEpdgSelector);

        when(mMockIPv4LinkAddress.getAddress()).thenReturn(mMockInet4Address);
        when(mMockIPv6LinkAddress.getAddress()).thenReturn(mMockInet6Address);

        mIwlanDataService = spy(new IwlanDataService());
        mIwlanDataService.setAppContext(mMockContext);
        mIwlanDataServiceProvider =
                (IwlanDataServiceProvider)
                        mIwlanDataService.onCreateDataServiceProvider(DEFAULT_SLOT_INDEX);
        mSpyIwlanDataServiceProvider = spy(mIwlanDataServiceProvider);
    }

    @After
    public void cleanUp() throws Exception {
        mStaticMockSession.finishMocking();
        mIwlanDataServiceProvider.close();
        if (mIwlanDataService != null) {
            mIwlanDataService.onDestroy();
        }
    }

    @Test
    public void testWifiOnAvailable() {
        IwlanNetworkMonitorCallback mNetworkMonitorCallback =
                mIwlanDataService.getNetworkMonitorCallback();

        mNetworkMonitorCallback.onAvailable(mMockNetwork);
        boolean ret = mIwlanDataService.isNetworkConnected(true, false);

        assertTrue(ret);
    }

    @Test
    public void testWifiOnLost() {
        mIwlanDataService.addIwlanDataServiceProvider(mMockIwlanDataServiceProvider);
        IwlanNetworkMonitorCallback mNetworkMonitorCallback =
                mIwlanDataService.getNetworkMonitorCallback();

        mNetworkMonitorCallback.onLost(mMockNetwork);
        boolean ret = mIwlanDataService.isNetworkConnected(true, false);

        assertFalse(ret);
        verify(mMockIwlanDataServiceProvider).forceCloseTunnelsInDeactivatingState();
        mIwlanDataService.removeDataServiceProvider(mMockIwlanDataServiceProvider);
    }

    @Test
    public void testRequestDataCallListPass() throws Exception {
        DataProfile dp = buildDataProfile();
        List<LinkAddress> mInternalAddressList;
        List<InetAddress> mDNSAddressList;
        List<InetAddress> mGatewayAddressList;
        List<InetAddress> mPCSFAddressList;

        latch = new CountDownLatch(1);
        IwlanDataServiceCallback callback = new IwlanDataServiceCallback("requestDataCallList");
        TunnelLinkProperties mLinkProperties =
                TunnelLinkPropertiesTest.createTestTunnelLinkProperties();
        mIwlanDataServiceProvider.setTunnelState(
                dp,
                new DataServiceCallback(callback),
                TunnelState.TUNNEL_UP,
                mLinkProperties,
                false,
                1);
        mIwlanDataServiceProvider.requestDataCallList(new DataServiceCallback(callback));
        latch.await(1, TimeUnit.SECONDS);

        assertEquals(mResultCode, DataServiceCallback.RESULT_SUCCESS);
        assertEquals(mResultDataCallList.size(), 1);
        for (DataCallResponse dataCallInfo : mResultDataCallList) {
            assertEquals(dataCallInfo.getId(), TEST_APN_NAME.hashCode());
            assertEquals(dataCallInfo.getLinkStatus(), DataCallResponse.LINK_STATUS_ACTIVE);
            assertEquals(dataCallInfo.getProtocolType(), ApnSetting.PROTOCOL_IPV4V6);
            assertEquals(dataCallInfo.getInterfaceName(), INTERFACE_NAME);

            mInternalAddressList = dataCallInfo.getAddresses();
            assertEquals(mInternalAddressList.size(), 1);
            for (LinkAddress mLinkAddress : mInternalAddressList) {
                assertEquals(mLinkAddress, new LinkAddress(InetAddress.getByName(IP_ADDRESS), 3));
            }

            mDNSAddressList = dataCallInfo.getDnsAddresses();
            assertEquals(mDNSAddressList.size(), 1);
            for (InetAddress mInetAddress : mDNSAddressList) {
                assertEquals(mInetAddress, InetAddress.getByName(DNS_ADDRESS));
            }

            mGatewayAddressList = dataCallInfo.getGatewayAddresses();
            assertEquals(mGatewayAddressList.size(), 1);
            for (InetAddress mInetAddress : mGatewayAddressList) {
                assertEquals(mInetAddress, Inet4Address.getByName(GATEWAY_ADDRESS));
            }

            mPCSFAddressList = dataCallInfo.getPcscfAddresses();
            assertEquals(mPCSFAddressList.size(), 1);
            for (InetAddress mInetAddress : mPCSFAddressList) {
                assertEquals(mInetAddress, InetAddress.getByName(PSCF_ADDRESS));
            }

            assertEquals(dataCallInfo.getMtuV4(), LINK_MTU);
            assertEquals(dataCallInfo.getMtuV6(), LINK_MTU);
        }
    }

    @Test
    public void testRequestDataCallListEmpty() throws Exception {
        latch = new CountDownLatch(1);
        IwlanDataServiceCallback callback = new IwlanDataServiceCallback("requestDataCallList");
        mIwlanDataServiceProvider.requestDataCallList(new DataServiceCallback(callback));
        latch.await(1, TimeUnit.SECONDS);

        assertEquals(mResultCode, DataServiceCallback.RESULT_SUCCESS);
        assertEquals(mResultDataCallList.size(), 0);
    }

    @Test
    public void testIwlanSetupDataCallWithInvalidArg() {
        mIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.UNKNOWN, /* AccessNetworkType */
                null, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                2, /* pdu session id */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);

        verify(mMockDataServiceCallback, timeout(1000).times(1))
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_ERROR_INVALID_ARG), isNull());
    }

    @Test
    public void testIwlanSetupDataCallWithIllegalState() {
        DataProfile dp = buildDataProfile();

        /* Wifi is not connected */
        mIwlanDataService.setNetworkConnected(
                false, mMockNetwork, IwlanDataService.Transport.UNSPECIFIED_NETWORK);

        mIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pdu session id */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);

        verify(mMockDataServiceCallback, timeout(1000).times(1))
                .onSetupDataCallComplete(
                        eq(5 /*DataServiceCallback.RESULT_ERROR_TEMPORARILY_UNAVAILABLE */), isNull());
    }

    @Test
    public void testIwlanDeactivateDataCallWithInvalidArg() {
        mIwlanDataServiceProvider.deactivateDataCall(
                0, /* cid */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                mMockDataServiceCallback);

        verify(mMockDataServiceCallback, timeout(1000).times(1))
                .onDeactivateDataCallComplete(eq(DataServiceCallback.RESULT_ERROR_INVALID_ARG));
    }

    @Test
    public void testIwlanSetupDataCallWithBringUpTunnel() {
        DataProfile dp = buildDataProfile();

        /* Wifi is connected */
        mIwlanDataService.setNetworkConnected(true, mMockNetwork, IwlanDataService.Transport.WIFI);

        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();

        mSpyIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pduSessionId */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);

        /* Check bringUpTunnel() is called. */
        verify(mMockEpdgTunnelManager, times(1))
                .bringUpTunnel(any(TunnelSetupRequest.class), any(IwlanTunnelCallback.class));

        /* Check callback result is RESULT_SUCCESS when onOpened() is called. */
        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onOpened(TEST_APN_NAME, mMockTunnelLinkProperties);
        verify(mMockDataServiceCallback, times(1))
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), any(DataCallResponse.class));
    }

    @Test
    public void testSliceInfoInclusionInDataCallResponse() throws Exception {
        DataProfile dp = buildDataProfile();

        /* Wifi is connected */
        mIwlanDataService.setNetworkConnected(true, mMockNetwork, IwlanDataService.Transport.WIFI);

        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();

        mSpyIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pduSessionId */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);

        /* Check bringUpTunnel() is called. */
        verify(mMockEpdgTunnelManager, times(1))
                .bringUpTunnel(any(TunnelSetupRequest.class), any(IwlanTunnelCallback.class));

        /* Check callback result is RESULT_SUCCESS when onOpened() is called. */
        TunnelLinkProperties tp = TunnelLinkPropertiesTest.createTestTunnelLinkProperties();

        ArgumentCaptor<DataCallResponse> dataCallResponseCaptor =
                ArgumentCaptor.forClass(DataCallResponse.class);

        mSpyIwlanDataServiceProvider.getIwlanTunnelCallback().onOpened(TEST_APN_NAME, tp);
        verify(mMockDataServiceCallback, times(1))
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), dataCallResponseCaptor.capture());

        /* check that sliceinfo is filled up and matches */
        DataCallResponse dataCallResponse = dataCallResponseCaptor.getValue();
        assertNotNull(dataCallResponse.getSliceInfo());
        assertEquals(dataCallResponse.getSliceInfo(), tp.sliceInfo().get());
    }

    @Test
    public void testIwlanDeactivateDataCallWithCloseTunnel() {
        DataProfile dp = buildDataProfile();

        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();

        mSpyIwlanDataServiceProvider.setTunnelState(
                dp, mMockDataServiceCallback, TunnelState.TUNNEL_IN_BRINGUP, null, false, 1);

        mSpyIwlanDataServiceProvider.deactivateDataCall(
                TEST_APN_NAME.hashCode() /* cid: hashcode() of "ims" */,
                DataService.REQUEST_REASON_NORMAL /* DataService.REQUEST_REASON_NORMAL */,
                mMockDataServiceCallback);

        /* Check closeTunnel() is called. */
        verify(mMockEpdgTunnelManager, times(1)).closeTunnel(eq(TEST_APN_NAME), anyBoolean());

        /* Check callback result is RESULT_SUCCESS when onClosed() is called. */
        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onClosed(TEST_APN_NAME, new IwlanError(IwlanError.NO_ERROR));
        verify(mMockDataServiceCallback, times(1))
                .onDeactivateDataCallComplete(eq(DataServiceCallback.RESULT_SUCCESS));
    }

    @Test
    public void testHandoverFailureModeNormal() {
        DataProfile dp = buildDataProfile();
        int setupDataReason = DataService.REQUEST_REASON_NORMAL;

        when(ErrorPolicyManager.getInstance(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(mMockErrorPolicyManager);
        when(mMockErrorPolicyManager.getCurrentRetryTimeMs(eq(TEST_APN_NAME))).thenReturn(5L);
        when(mMockErrorPolicyManager.getDataFailCause(eq(TEST_APN_NAME)))
                .thenReturn(DataFailCause.USER_AUTHENTICATION);

        mSpyIwlanDataServiceProvider.setTunnelState(
                dp,
                mMockDataServiceCallback,
                TunnelState.TUNNEL_IN_BRINGUP,
                null,
                (setupDataReason == DataService.REQUEST_REASON_HANDOVER),
                1);

        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onClosed(TEST_APN_NAME, new IwlanError(IwlanError.NO_ERROR));

        ArgumentCaptor<DataCallResponse> dataCallResponseCaptor =
                ArgumentCaptor.forClass(DataCallResponse.class);

        verify(mMockDataServiceCallback, times(1))
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), dataCallResponseCaptor.capture());

        DataCallResponse dataCallResponse = dataCallResponseCaptor.getValue();
        assertEquals(
                dataCallResponse.getHandoverFailureMode(),
                DataCallResponse.HANDOVER_FAILURE_MODE_NO_FALLBACK_RETRY_SETUP_NORMAL);
        assertEquals(dataCallResponse.getCause(), DataFailCause.USER_AUTHENTICATION);
        assertEquals(dataCallResponse.getRetryDurationMillis(), 5L);
    }

    @Test
    public void testHandoverFailureModeHandover() {
        DataProfile dp = buildDataProfile();
        int setupDataReason = DataService.REQUEST_REASON_HANDOVER;

        when(ErrorPolicyManager.getInstance(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(mMockErrorPolicyManager);
        when(mMockErrorPolicyManager.getCurrentRetryTimeMs(eq(TEST_APN_NAME))).thenReturn(-1L);
        when(mMockErrorPolicyManager.getDataFailCause(eq(TEST_APN_NAME)))
                .thenReturn(DataFailCause.ERROR_UNSPECIFIED);

        mSpyIwlanDataServiceProvider.setTunnelState(
                dp,
                mMockDataServiceCallback,
                TunnelState.TUNNEL_IN_BRINGUP,
                null,
                (setupDataReason == DataService.REQUEST_REASON_HANDOVER),
                1);

        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onClosed(TEST_APN_NAME, new IwlanError(IwlanError.NO_ERROR));

        ArgumentCaptor<DataCallResponse> dataCallResponseCaptor =
                ArgumentCaptor.forClass(DataCallResponse.class);

        verify(mMockDataServiceCallback, times(1))
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), dataCallResponseCaptor.capture());

        DataCallResponse dataCallResponse = dataCallResponseCaptor.getValue();
        assertEquals(
                dataCallResponse.getHandoverFailureMode(),
                DataCallResponse.HANDOVER_FAILURE_MODE_NO_FALLBACK_RETRY_HANDOVER);
        assertEquals(dataCallResponse.getCause(), DataFailCause.ERROR_UNSPECIFIED);
        assertEquals(dataCallResponse.getRetryDurationMillis(), -1L);
    }

    @Test
    public void testDnsPrefetching() throws Exception {
        IwlanNetworkMonitorCallback mNetworkMonitorCallback =
                mIwlanDataService.getNetworkMonitorCallback();
        /* Wifi is connected */
        mIwlanDataService.setNetworkConnected(true, mMockNetwork, IwlanDataService.Transport.WIFI);

        List<LinkAddress> linkAddresses = new ArrayList<>();
        linkAddresses.add(mMockIPv4LinkAddress);

        when(mMockLinkProperties.getLinkAddresses()).thenReturn(linkAddresses);
        mNetworkMonitorCallback.onLinkPropertiesChanged(mMockNetwork, mMockLinkProperties);

        mIwlanDataServiceProvider
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();
        sleep(1000);

        mIwlanDataServiceProvider
                .mHandler
                .obtainMessage(IwlanEventListener.WIFI_CALLING_ENABLE_EVENT)
                .sendToTarget();
        sleep(1000);

        linkAddresses.add(mMockIPv6LinkAddress);

        when(mMockLinkProperties.getLinkAddresses()).thenReturn(linkAddresses);
        mNetworkMonitorCallback.onLinkPropertiesChanged(mMockNetwork, mMockLinkProperties);

        /* Prefetching will be triggered twice.
           1. Network connected, CarrierConfig ready, WifiCallingSetting enabled
           2. Connection ipFamily changed.
        */
        verify(mMockEpdgSelector, times(2))
                .getValidatedServerList(
                        eq(0),
                        eq(EpdgSelector.PROTO_FILTER_IPV4V6),
                        eq(false),
                        eq(false),
                        eq(mMockNetwork),
                        isNull());
        verify(mMockEpdgSelector, times(2))
                .getValidatedServerList(
                        eq(0),
                        eq(EpdgSelector.PROTO_FILTER_IPV4V6),
                        eq(false),
                        eq(true),
                        eq(mMockNetwork),
                        isNull());
    }

    private void sleep(long time) {
        try {
            Thread.sleep(time);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private DataProfile buildDataProfile() {
        DataProfile dp =
                new DataProfile.Builder()
                        .setProfileId(1)
                        .setApn(TEST_APN_NAME)
                        .setProtocolType(ApnSetting.PROTOCOL_IPV4V6) // IPv4v6
                        .setAuthType(0) // none
                        .setUserName("")
                        .setPassword("")
                        .setType(1) // 3gpp
                        // .setMaxConnectionsTime(1)
                        // .setMaxConnections(3)
                        // .setWaitTime(10)
                        .enable(true)
                        .setSupportedApnTypesBitmask(ApnSetting.TYPE_IMS)
                        .setRoamingProtocolType(ApnSetting.PROTOCOL_IPV4V6) // IPv4v6
                        .setBearerBitmask((int) TelephonyManager.NETWORK_TYPE_BITMASK_IWLAN)
                        .setPersistent(true)
                        .setPreferred(true)
                        .build();
        return dp;
    }

    @Test
    public void testIwlanSetupDataCallWithCellularAndCstDisabled() {
        DataProfile dp = buildDataProfile();

        /* Mobile is connected */
        mIwlanDataService.setNetworkConnected(
                true, mMockNetwork, IwlanDataService.Transport.MOBILE);

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(false);
        lenient()
                .when(IwlanHelper.isDefaultDataSlot(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);

        mIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pdu session id */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);

        verify(mMockDataServiceCallback, timeout(1000).times(1))
                .onSetupDataCallComplete(
                        eq(5 /* DataServiceCallback.RESULT_ERROR_TEMPORARILY_UNAVAILABLE */), isNull());
    }

    @Test
    public void testIwlanSetupDataCallWithCellularAndCstEnabled() {
        DataProfile dp = buildDataProfile();

        /* Clear state */
        mIwlanDataService.setNetworkConnected(
                false, mMockNetwork, IwlanDataService.Transport.UNSPECIFIED_NETWORK);

        /* Mobile is connected */
        mIwlanDataService.setNetworkConnected(
                true, mMockNetwork, IwlanDataService.Transport.MOBILE);

        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();

        lenient()
                .when(
                        IwlanHelper.isCrossSimCallingEnabled(
                                eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(true);
        lenient()
                .when(IwlanHelper.isDefaultDataSlot(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(false);

        mSpyIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pduSessionId */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);

        /* Check bringUpTunnel() is called. */
        verify(mMockEpdgTunnelManager, times(1))
                .bringUpTunnel(any(TunnelSetupRequest.class), any(IwlanTunnelCallback.class));

        /* Check callback result is RESULT_SUCCESS when onOpened() is called. */
        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onOpened(TEST_APN_NAME, mMockTunnelLinkProperties);
        verify(mMockDataServiceCallback, times(1))
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), any(DataCallResponse.class));
    }

    @Test
    public void testIwlanTunnelStatsFailureCounts() {
        DataProfile dp = buildDataProfile();

        mIwlanDataService.setNetworkConnected(true, mMockNetwork, IwlanDataService.Transport.WIFI);
        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();
        when(ErrorPolicyManager.getInstance(eq(mMockContext), eq(DEFAULT_SLOT_INDEX)))
                .thenReturn(mMockErrorPolicyManager);

        long count = 3L;
        for (int i = 0; i < count; i++) {
            mockTunnelSetupFail(dp);
            sleep(1000);
        }

        IwlanDataServiceProvider.IwlanDataTunnelStats stats =
                mSpyIwlanDataServiceProvider.getTunnelStats();
        long result = stats.mTunnelSetupFailureCounts.get(TEST_APN_NAME);
        assertEquals(result, count);
    }

    @Test
    public void testIwlanTunnelStatsUnsolDownCounts() {
        DataProfile dp = buildDataProfile();

        mIwlanDataService.setNetworkConnected(true, mMockNetwork, IwlanDataService.Transport.WIFI);
        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();

        long count = 3L;
        for (int i = 0; i < count; i++) {
            mockTunnelSetupSuccess(dp, 0);
            mockUnsolTunnelDown();
        }

        IwlanDataServiceProvider.IwlanDataTunnelStats stats =
                mSpyIwlanDataServiceProvider.getTunnelStats();
        long result = stats.mUnsolTunnelDownCounts.get(TEST_APN_NAME);
        assertEquals(result, count);
    }

    @Test
    public void testIwlanTunnelStats() {
        DataProfile dp = buildDataProfile();

        mIwlanDataService.setNetworkConnected(true, mMockNetwork, IwlanDataService.Transport.WIFI);
        doReturn(mMockEpdgTunnelManager).when(mSpyIwlanDataServiceProvider).getTunnelManager();

        LongSummaryStatistics tunnelSetupSuccessStats = new LongSummaryStatistics();
        LongSummaryStatistics tunnelUpStats = new LongSummaryStatistics();

        Date beforeSetup = Calendar.getInstance().getTime();
        mockTunnelSetupSuccess(dp, 0);
        Date tunnelUp = Calendar.getInstance().getTime();
        mockDeactivateTunnelDown(0);
        Date tunnelDown = Calendar.getInstance().getTime();
        tunnelSetupSuccessStats.accept(tunnelUp.getTime() - beforeSetup.getTime());
        tunnelUpStats.accept(tunnelDown.getTime() - tunnelUp.getTime());

        beforeSetup = Calendar.getInstance().getTime();
        mockTunnelSetupSuccess(dp, 1000);
        tunnelUp = Calendar.getInstance().getTime();
        mockDeactivateTunnelDown(3000);
        tunnelDown = Calendar.getInstance().getTime();
        tunnelSetupSuccessStats.accept(tunnelUp.getTime() - beforeSetup.getTime());
        tunnelUpStats.accept(tunnelDown.getTime() - tunnelUp.getTime());

        beforeSetup = Calendar.getInstance().getTime();
        mockTunnelSetupSuccess(dp, 600);
        tunnelUp = Calendar.getInstance().getTime();
        mockDeactivateTunnelDown(500);
        tunnelDown = Calendar.getInstance().getTime();
        tunnelSetupSuccessStats.accept(tunnelUp.getTime() - beforeSetup.getTime());
        tunnelUpStats.accept(tunnelDown.getTime() - tunnelUp.getTime());

        IwlanDataServiceProvider.IwlanDataTunnelStats stats =
                mSpyIwlanDataServiceProvider.getTunnelStats();
        LongSummaryStatistics finalSetupStats = stats.mTunnelSetupSuccessStats.get(TEST_APN_NAME);
        LongSummaryStatistics finalUpStats = stats.mTunnelUpStats.get(TEST_APN_NAME);

        assertEquals(finalSetupStats.getAverage(), tunnelSetupSuccessStats.getAverage(), 100);
        assertEquals(finalSetupStats.getCount(), tunnelSetupSuccessStats.getCount());
        assertEquals(finalSetupStats.getMax(), tunnelSetupSuccessStats.getMax(), 100);

        assertEquals(finalUpStats.getAverage(), tunnelUpStats.getAverage(), 100);
        assertEquals(finalUpStats.getCount(), tunnelUpStats.getCount());
        assertEquals(finalUpStats.getMax(), tunnelUpStats.getMax(), 100);
    }

    private void mockTunnelSetupFail(DataProfile dp) {
        mSpyIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pduSessionId */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);
        doReturn(true)
                .when(mMockEpdgTunnelManager)
                .bringUpTunnel(any(TunnelSetupRequest.class), any(IwlanTunnelCallback.class));

        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onClosed(TEST_APN_NAME, new IwlanError(IwlanError.IKE_INTERNAL_IO_EXCEPTION));
        verify(mMockDataServiceCallback, atLeastOnce())
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), any(DataCallResponse.class));
    }

    private void mockTunnelSetupSuccess(DataProfile dp, long sleepTime) {
        mSpyIwlanDataServiceProvider.setupDataCall(
                AccessNetworkType.IWLAN, /* AccessNetworkType */
                dp, /* dataProfile */
                false, /* isRoaming */
                true, /* allowRoaming */
                DataService.REQUEST_REASON_NORMAL, /* DataService.REQUEST_REASON_NORMAL */
                null, /* LinkProperties */
                1, /* pduSessionId */
                null, /* sliceInfo */
                null, /* trafficDescriptor */
                true, /* matchAllRuleAllowed */
                mMockDataServiceCallback);
        doReturn(true)
                .when(mMockEpdgTunnelManager)
                .bringUpTunnel(any(TunnelSetupRequest.class), any(IwlanTunnelCallback.class));

        sleep(sleepTime);

        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onOpened(TEST_APN_NAME, mMockTunnelLinkProperties);
        verify(mMockDataServiceCallback, atLeastOnce())
                .onSetupDataCallComplete(
                        eq(DataServiceCallback.RESULT_SUCCESS), any(DataCallResponse.class));
    }

    private void mockUnsolTunnelDown() {
        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onClosed(TEST_APN_NAME, new IwlanError(IwlanError.IKE_INTERNAL_IO_EXCEPTION));
    }

    private void mockDeactivateTunnelDown(long sleepTime) {
        mSpyIwlanDataServiceProvider.deactivateDataCall(
                TEST_APN_NAME.hashCode() /* cid: hashcode() of "ims" */,
                DataService.REQUEST_REASON_NORMAL /* DataService.REQUEST_REASON_NORMAL */,
                mMockDataServiceCallback);
        verify(mMockEpdgTunnelManager, atLeastOnce()).closeTunnel(eq(TEST_APN_NAME), anyBoolean());

        sleep(sleepTime);

        mSpyIwlanDataServiceProvider
                .getIwlanTunnelCallback()
                .onClosed(TEST_APN_NAME, new IwlanError(IwlanError.NO_ERROR));
        verify(mMockDataServiceCallback, atLeastOnce())
                .onDeactivateDataCallComplete(eq(DataServiceCallback.RESULT_SUCCESS));
    }
}
