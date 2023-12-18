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

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.*;
import static org.mockito.Mockito.mock;
import static org.mockito.Mockito.spy;

import android.content.ContentResolver;
import android.content.Context;
import android.content.res.AssetManager;
import android.net.ipsec.ike.exceptions.IkeProtocolException;
import android.os.PersistableBundle;
import android.telephony.CarrierConfigManager;
import android.telephony.DataFailCause;
import android.telephony.SubscriptionInfo;
import android.telephony.SubscriptionManager;
import android.telephony.TelephonyManager;
import android.telephony.data.DataService;

import androidx.test.InstrumentationRegistry;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;
import org.mockito.quality.Strictness;

import java.io.InputStream;
import java.util.Map;

public class ErrorPolicyManagerTest {
    private static final String TAG = "ErrorPolicyManagerTest";

    // @Rule public final MockitoRule mockito = MockitoJUnit.rule();

    private ErrorPolicyManager mErrorPolicyManager;
    private static final int DEFAULT_SLOT_INDEX = 0;
    private static final int DEFAULT_SUBID = 0;
    private static final int TEST_CARRIER_ID = 1;

    @Mock private Context mMockContext;
    @Mock CarrierConfigManager mMockCarrierConfigManager;
    @Mock SubscriptionManager mMockSubscriptionManager;
    @Mock TelephonyManager mMockTelephonyManager;
    @Mock SubscriptionInfo mMockSubscriptionInfo;
    @Mock DataService.DataServiceProvider mMockDataServiceProvider;
    @Mock private ContentResolver mMockContentResolver;
    MockitoSession mStaticMockSession;

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        mStaticMockSession =
                mockitoSession()
                        .mockStatic(IwlanDataService.class)
                        .strictness(Strictness.LENIENT)
                        .startMocking();
        when(IwlanDataService.getDataServiceProvider(anyInt()))
                .thenReturn(mMockDataServiceProvider);
        AssetManager mockAssetManager = mock(AssetManager.class);
        Context context = InstrumentationRegistry.getTargetContext();
        InputStream is = context.getResources().getAssets().open("defaultiwlanerrorconfig.json");
        doReturn(mockAssetManager).when(mMockContext).getAssets();
        doReturn(is).when(mockAssetManager).open(any());
        setupMockForCarrierConfig(null);
        mErrorPolicyManager = spy(ErrorPolicyManager.getInstance(mMockContext, DEFAULT_SLOT_INDEX));
    }

    @After
    public void cleanUp() throws Exception {
        mStaticMockSession.finishMocking();
        mErrorPolicyManager.releaseInstance();
    }

    private static IwlanError buildIwlanIkeProtocolError(int errorCode, byte[] errorData) {
        final IkeProtocolException exception = mock(IkeProtocolException.class);
        when(exception.getErrorType()).thenReturn(errorCode);
        when(exception.getErrorData()).thenReturn(errorData);
        return new IwlanError(exception);
    }

    private static IwlanError buildIwlanIkeProtocolError(int errorCode) {
        return buildIwlanIkeProtocolError(errorCode, new byte[0]);
    }

    private static IwlanError buildIwlanIkeAuthFailedError() {
        return buildIwlanIkeProtocolError(IkeProtocolException.ERROR_TYPE_AUTHENTICATION_FAILED);
    }

    private static IwlanError buildIwlanIkeChildSaNotFoundError() {
        return buildIwlanIkeProtocolError(IkeProtocolException.ERROR_TYPE_CHILD_SA_NOT_FOUND);
    }

    @Test
    public void testValidCarrierConfig() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34", "9000-9050"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_ENABLE_EVENT"})
                        + "}]"
                        + "}]";

        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4,8,16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(8, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(16, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(86400, time);

        // Validate the range error detail.
        iwlanError = buildIwlanIkeProtocolError(9030, new byte[] {0x00, 0x01});
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(8, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(16, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(86400, time);

        // GENERIC_PROTOCOL_ERROR_TYPE - SERVER_SELECTION_FAILED and retryArray = 0
        iwlanError = new IwlanError(IwlanError.EPDG_SELECTOR_SERVER_SELECTION_FAILED);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(0, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(86400, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(86400, time);

        // Fallback case GENERIC_PROTOCOL_ERROR_TYPE(44) and retryArray is 5, 10, -1 as in
        // DEFAULT_CONFIG
        iwlanError = buildIwlanIkeChildSaNotFoundError();
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(5, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
    }

    @Test
    public void testInvalidCarrierConfig() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"WRONG_ERROR_DETAIL"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}]"
                        + "}]";

        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 5, 10, 15 as it will fallback due to failed
        // parsing
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(5, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
    }

    @Test
    public void testChoosingFallbackPolicy() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"*"},
                                new String[] {"0"},
                                new String[] {"APM_ENABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        mErrorPolicyManager.logErrorPolicies();

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4, 8, 16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(8, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(16, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(86400, time);

        // IKE_PROTOCOL_ERROR_TYPE(44) and retryArray = 0 as it will fallback to
        // IKE_PROTOCOL_ERROR_TYPE generic fallback first.
        iwlanError = buildIwlanIkeChildSaNotFoundError();
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(0, time);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(86400, time);
    }

    @Test
    public void testCanBringUpTunnel() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_ENABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4,8,16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);

        boolean bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertFalse(bringUpTunnel);

        sleep(4000);

        bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertTrue(bringUpTunnel);

        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(8, time);

        bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertFalse(bringUpTunnel);
    }

    @Test
    public void testNoErrorScenario() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_ENABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4,8,16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);

        // report no error
        iwlanError = new IwlanError(IwlanError.NO_ERROR);
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(-1, time);

        // Check whether the error is cleared after NO_ERROR is reported
        boolean bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertTrue(bringUpTunnel);
    }

    @Test
    public void testWifiDisableUnthrottle() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"6", "12", "24"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_DISABLE_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_DISABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();
        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 6, 12, 24
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(6, time);

        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.WIFI_DISABLE_EVENT)
                .sendToTarget();
        sleep(500);
        verify(mMockDataServiceProvider, times(1)).notifyApnUnthrottled(eq(apn));

        boolean bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertTrue(bringUpTunnel);

        iwlanError = buildIwlanIkeAuthFailedError();
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(6, time);
    }

    @Test
    public void testWFCDisableUnthrottle() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"6", "12", "24"},
                                new String[] {"WIFI_CALLING_DISABLE_EVENT", "WIFI_DISABLE_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_DISABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();
        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 6, 12, 24
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(6, time);

        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.WIFI_CALLING_DISABLE_EVENT)
                .sendToTarget();
        sleep(500);
        verify(mMockDataServiceProvider, times(1)).notifyApnUnthrottled(eq(apn));

        boolean bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertTrue(bringUpTunnel);

        iwlanError = buildIwlanIkeAuthFailedError();
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(6, time);
    }

    @Test
    public void testAPMUnthrottle() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_DISABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();
        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4,8,16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);

        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.APM_ENABLE_EVENT)
                .sendToTarget();
        sleep(500);
        verify(mMockDataServiceProvider, times(1)).notifyApnUnthrottled(eq(apn));

        boolean bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertTrue(bringUpTunnel);

        iwlanError = buildIwlanIkeAuthFailedError();
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(4, time);
    }

    @Test
    public void testGetDataFailCauseRetryTime() throws Exception {
        String apn1 = "ims";
        String apn2 = "mms";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn1
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"4", "8", "16"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_ENABLE_EVENT"})
                        + "}]"
                        + "}]";
        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4,8,16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn1, iwlanError);
        assertEquals(4, time);

        iwlanError =
                buildIwlanIkeProtocolError(
                        8192 /*PDN_CONNECTION_REJECTION*/, new byte[] {0x00, 0x01});
        time = mErrorPolicyManager.reportIwlanError(apn2, iwlanError);
        assertEquals(5, time);

        int failCause = mErrorPolicyManager.getDataFailCause(apn1);
        assertEquals(DataFailCause.IWLAN_IKEV2_AUTH_FAILURE, failCause);

        failCause = mErrorPolicyManager.getDataFailCause(apn2);
        assertEquals(DataFailCause.IWLAN_PDN_CONNECTION_REJECTION, failCause);

        long retryTime =
                Math.round((double) mErrorPolicyManager.getCurrentRetryTimeMs(apn1) / 1000);
        assertEquals(4, retryTime);

        retryTime = Math.round((double) mErrorPolicyManager.getCurrentRetryTimeMs(apn2) / 1000);
        assertEquals(5, retryTime);
    }

    @Test
    public void testBackOffTime() throws Exception {
        String apn = "ims";
        String config =
                "[{"
                        + "\"ApnName\": \""
                        + apn
                        + "\","
                        + "\"ErrorTypes\": [{"
                        + getErrorTypeInJSON(
                                "IKE_PROTOCOL_ERROR_TYPE",
                                new String[] {"24", "34"},
                                new String[] {"10", "15", "20"},
                                new String[] {"APM_ENABLE_EVENT", "WIFI_AP_CHANGED_EVENT"})
                        + "}, {"
                        + getErrorTypeInJSON(
                                "GENERIC_ERROR_TYPE",
                                new String[] {"SERVER_SELECTION_FAILED"},
                                new String[] {"0"},
                                new String[] {"APM_ENABLE_EVENT"})
                        + "}]"
                        + "}]";

        PersistableBundle bundle = new PersistableBundle();
        bundle.putString(ErrorPolicyManager.KEY_ERROR_POLICY_CONFIG_STRING, config);
        setupMockForCarrierConfig(bundle);
        mErrorPolicyManager
                .mHandler
                .obtainMessage(IwlanEventListener.CARRIER_CONFIG_CHANGED_EVENT)
                .sendToTarget();

        sleep(1000);

        // IKE_PROTOCOL_ERROR_TYPE(24) and retryArray = 4,8,16
        IwlanError iwlanError = buildIwlanIkeAuthFailedError();
        long time = mErrorPolicyManager.reportIwlanError(apn, iwlanError, 2);

        time = Math.round((double) mErrorPolicyManager.getCurrentRetryTimeMs(apn) / 1000);
        assertEquals(time, 2);

        // sleep for 2 seconds and make sure that we can bring up tunnel after 2 secs
        // as back off time - 2 secs should override the retry time in policy - 10 secs
        sleep(2000);
        boolean bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertTrue(bringUpTunnel);

        // test whether the same error reported later uses the right policy
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);

        bringUpTunnel = mErrorPolicyManager.canBringUpTunnel(apn);
        assertFalse(bringUpTunnel);

        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError, 5);
        time = Math.round((double) mErrorPolicyManager.getCurrentRetryTimeMs(apn) / 1000);
        assertEquals(time, 5);

        // test whether the same error reported later starts from the beginning of retry array
        time = mErrorPolicyManager.reportIwlanError(apn, iwlanError);
        assertEquals(10, time);
    }

    @Test
    public void testErrorStats() throws Exception {
        String apn1 = "ims";
        String apn2 = "mms";

        setupMockForCarrierConfig(null);

        IwlanError iwlanError1 = buildIwlanIkeAuthFailedError();
        long ikeAuthCountApn1 = 4L;
        long ikeAuthCountApn2 = 5L;
        for (int i = 0; i < ikeAuthCountApn1; i++) {
            mErrorPolicyManager.reportIwlanError(apn1, iwlanError1);
        }
        for (int i = 0; i < ikeAuthCountApn2; i++) {
            mErrorPolicyManager.reportIwlanError(apn2, iwlanError1);
        }

        long serverSelectionCountApn1 = 3L;
        long serverSelectionCountApn2 = 6L;
        IwlanError iwlanError2 = new IwlanError(IwlanError.EPDG_SELECTOR_SERVER_SELECTION_FAILED);
        for (int i = 0; i < serverSelectionCountApn1; i++) {
            mErrorPolicyManager.reportIwlanError(apn1, iwlanError2);
        }
        // calling backoff timer api
        for (int i = 0; i < serverSelectionCountApn2; i++) {
            mErrorPolicyManager.reportIwlanError(apn2, iwlanError2, 3);
        }

        Map<String, Long> apn1Stats = mErrorPolicyManager.getErrorStats().mStats.get(apn1);
        Map<String, Long> apn2Stats = mErrorPolicyManager.getErrorStats().mStats.get(apn2);

        long resultAuthApn1 = apn1Stats.get(iwlanError1.toString());
        long resultAuthApn2 = apn2Stats.get(iwlanError1.toString());
        long resultServerApn1 = apn1Stats.get(iwlanError2.toString());
        long resultServerApn2 = apn2Stats.get(iwlanError2.toString());
        assertEquals(resultAuthApn1, ikeAuthCountApn1);
        assertEquals(resultAuthApn2, ikeAuthCountApn2);
        assertEquals(resultServerApn1, serverSelectionCountApn1);
        assertEquals(resultServerApn2, serverSelectionCountApn2);
    }

    private String getErrorTypeInJSON(
            String ErrorType,
            String[] errorDetails,
            String[] retryArray,
            String[] unthrottlingEvents) {
        return "\"ErrorType\": \""
                + ErrorType
                + "\","
                + "\"ErrorDetails\": [\""
                + String.join("\", \"", errorDetails)
                + "\"],"
                + "\"RetryArray\": [\""
                + String.join("\", \"", retryArray)
                + "\"],"
                + "\"UnthrottlingEvents\": [\""
                + String.join("\", \"", unthrottlingEvents)
                + "\"]";
    }

    private void sleep(long time) {
        try {
            Thread.sleep(time);
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    private void setupMockForCarrierConfig(PersistableBundle bundle) {
        doReturn(mMockCarrierConfigManager)
                .when(mMockContext)
                .getSystemService(eq(CarrierConfigManager.class));
        doReturn(mMockSubscriptionManager)
                .when(mMockContext)
                .getSystemService(eq(SubscriptionManager.class));
        doReturn(mMockTelephonyManager).when(mMockContext).getSystemService(TelephonyManager.class);
        doReturn(mMockTelephonyManager)
                .when(mMockTelephonyManager)
                .createForSubscriptionId(anyInt());
        doReturn(TEST_CARRIER_ID).when(mMockTelephonyManager).getSimCarrierId();
        SubscriptionInfo mockSubInfo = mock(SubscriptionInfo.class);
        doReturn(mockSubInfo)
                .when(mMockSubscriptionManager)
                .getActiveSubscriptionInfoForSimSlotIndex(DEFAULT_SLOT_INDEX);
        doReturn(DEFAULT_SUBID).when(mockSubInfo).getSubscriptionId();
        doReturn(bundle).when(mMockCarrierConfigManager).getConfigForSubId(DEFAULT_SLOT_INDEX);
        when(mMockContext.getContentResolver()).thenReturn(mMockContentResolver);
    }
}
