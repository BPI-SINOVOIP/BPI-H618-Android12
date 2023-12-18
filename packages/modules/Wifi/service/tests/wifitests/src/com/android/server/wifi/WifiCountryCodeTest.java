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

package com.android.server.wifi;

import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_PRIMARY;
import static com.android.server.wifi.ActiveModeManager.ROLE_CLIENT_SECONDARY_LONG_LIVED;
import static com.android.server.wifi.WifiSettingsConfigStore.WIFI_DEFAULT_COUNTRY_CODE;

import static org.junit.Assert.*;
import static org.mockito.Mockito.*;
import static org.mockito.Mockito.doAnswer;

import android.app.test.MockAnswerUtil.AnswerWithArguments;
import android.content.Context;
import android.telephony.TelephonyManager;

import androidx.test.filters.SmallTest;

import com.android.wifi.resources.R;

import org.junit.Before;
import org.junit.Test;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;

import java.io.PrintWriter;
import java.io.StringWriter;
import java.util.Locale;

/**
 * Unit tests for {@link com.android.server.wifi.WifiCountryCode}.
 */
@SmallTest
public class WifiCountryCodeTest extends WifiBaseTest {

    private static final String TAG = "WifiCountryCodeTest";
    private static final String TEST_COUNTRY_CODE = "JP";
    private String mDefaultCountryCode = "US";
    private String mTelephonyCountryCode = "JP";
    private boolean mRevertCountryCodeOnCellularLoss = true;
    @Mock Context mContext;
    MockResources mResources = new MockResources();
    @Mock TelephonyManager mTelephonyManager;
    @Mock ActiveModeWarden mActiveModeWarden;
    @Mock ConcreteClientModeManager mClientModeManager;
    @Mock ClientModeImplMonitor mClientModeImplMonitor;
    @Mock WifiNative mWifiNative;
    @Mock WifiSettingsConfigStore mSettingsConfigStore;
    private WifiCountryCode mWifiCountryCode;

    @Captor
    private ArgumentCaptor<ActiveModeWarden.ModeChangeCallback> mModeChangeCallbackCaptor;
    @Captor
    private ArgumentCaptor<ClientModeImplListener> mClientModeImplListenerCaptor;
    @Captor
    private ArgumentCaptor<WifiCountryCode.ChangeListener> mChangeListenerCaptor;
    @Captor
    private ArgumentCaptor<String> mSetCountryCodeCaptor;

    /**
     * Setup test.
     */
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);

        when(mClientModeManager.getRole()).thenReturn(ROLE_CLIENT_PRIMARY);
        when(mClientModeManager.setCountryCode(anyString())).thenReturn(true);
        when(mContext.getSystemService(Context.TELEPHONY_SERVICE))
                .thenReturn(mTelephonyManager);

        doAnswer((invocation) -> {
            mChangeListenerCaptor.getValue()
                    .onDriverCountryCodeChanged(mSetCountryCodeCaptor.getValue());
            return true;
        }).when(mClientModeManager).setCountryCode(
                    mSetCountryCodeCaptor.capture());

        doAnswer(new AnswerWithArguments() {
            public void answer(WifiSettingsConfigStore.Key<String> key, Object countryCode) {
                when(mSettingsConfigStore.get(WIFI_DEFAULT_COUNTRY_CODE))
                        .thenReturn((String) countryCode);
            }
        }).when(mSettingsConfigStore).put(eq(WIFI_DEFAULT_COUNTRY_CODE), any(String.class));

        when(mSettingsConfigStore.get(WIFI_DEFAULT_COUNTRY_CODE)).thenReturn(mDefaultCountryCode);

        createWifiCountryCode();
    }

    private void createWifiCountryCode() {
        mResources.setBoolean(R.bool.config_wifi_revert_country_code_on_cellular_loss,
                mRevertCountryCodeOnCellularLoss);
        when(mContext.getResources()).thenReturn(mResources);
        mWifiCountryCode = new WifiCountryCode(
                mContext,
                mActiveModeWarden,
                mClientModeImplMonitor,
                mWifiNative,
                mSettingsConfigStore);
        verify(mActiveModeWarden, atLeastOnce()).registerModeChangeCallback(
                    mModeChangeCallbackCaptor.capture());
        verify(mClientModeImplMonitor, atLeastOnce()).registerListener(
                mClientModeImplListenerCaptor.capture());
        verify(mWifiNative, atLeastOnce()).registerCountryCodeEventListener(
                mChangeListenerCaptor.capture());
    }

    /**
     * Test if we do not receive country code from Telephony.
     * @throws Exception
     */
    @Test
    public void useDefaultCountryCode() throws Exception {
        // Supplicant started.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        // Wifi get L2 connected.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);
        verify(mClientModeManager).setCountryCode(anyString());
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());
    }

    /**
     * Test that we read the country code from telephony at bootup.
     * @throws Exception
     */
    @Test
    public void useTelephonyCountryCodeOnBootup() throws Exception {
        when(mTelephonyManager.getNetworkCountryIso()).thenReturn(mTelephonyCountryCode);
        // Supplicant started.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        // Wifi get L2 connected.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);
        verify(mClientModeManager).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());
    }

    /**
     * Test if we receive country code from Telephony before supplicant starts.
     * @throws Exception
     */
    @Test
    public void useTelephonyCountryCodeOnChange() throws Exception {
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        assertEquals(null, mWifiCountryCode.getCountryCodeSentToDriver());
        // Supplicant started.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        // Wifi get L2 connected.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);
        verify(mClientModeManager).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());
    }

    /**
     * Test if we receive country code from Telephony after supplicant starts.
     * @throws Exception
     */
    @Test
    public void telephonyCountryCodeChangeAfterSupplicantStarts() throws Exception {
        // Start in scan only mode.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Supplicant starts.
        when(mClientModeManager.getRole()).thenReturn(ROLE_CLIENT_PRIMARY);
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerRoleChanged(mClientModeManager);
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Telephony country code arrives.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        // Wifi get L2 connected.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);

        verify(mClientModeManager, times(3)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());
    }


    /**
     * Test if we receive country code from Telephony after supplicant stop.
     * @throws Exception
     */
    @Test
    public void telephonyCountryCodeChangeAfterSupplicantStop() throws Exception {
        // Start in scan only mode.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Supplicant starts.
        when(mClientModeManager.getRole()).thenReturn(ROLE_CLIENT_PRIMARY);
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerRoleChanged(mClientModeManager);
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Telephony country code arrives.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        // Wifi get L2 connected.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);

        verify(mClientModeManager, times(3)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Remove mode manager.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerRemoved(mClientModeManager);

        // Send Telephony country code again - should be ignored.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        verify(mClientModeManager, times(3)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Now try removing the mode manager again - should not crash.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerRemoved(mClientModeManager);
    }

    /**
     * Test if we receive country code from Telephony after we get L2 connected.
     * @throws Exception
     */
    @Test
    public void telephonyCountryCodeChangeAfterL2Connected() throws Exception {
        // Supplicant starts.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        // Wifi get L2 connected.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);
        // Telephony country code arrives.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        // Telephony coutry code won't be applied at this time.
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        mClientModeImplListenerCaptor.getValue().onConnectionEnd(mClientModeManager);
        // Telephony coutry is applied after supplicant is ready.
        verify(mClientModeManager, times(2)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());
    }

    /**
     * Test if we receive country code from Telephony after we get L2 connected on 2 STA interfaces.
     * @throws Exception
     */
    @Test
    public void telephonyCountryCodeChangeAfterL2ConnectedOnTwoClientModeManager()
            throws Exception {
        // Primary CMM
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        // Wifi get L2 connected on the primary.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(mClientModeManager);
        // Telephony country code arrives.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        // Telephony country code won't be applied at this time.
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Create secondary CMM
        ConcreteClientModeManager secondaryClientModeManager =
                mock(ConcreteClientModeManager.class);
        when(mClientModeManager.getRole()).thenReturn(ROLE_CLIENT_SECONDARY_LONG_LIVED);
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(secondaryClientModeManager);
        // Wifi get L2 connected on the secondary.
        mClientModeImplListenerCaptor.getValue().onConnectionStart(secondaryClientModeManager);

        // Telephony country code still not applied.
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Disconnection on primary
        mClientModeImplListenerCaptor.getValue().onConnectionEnd(mClientModeManager);

        // Telephony country code still not applied.
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());

        // Disconnection on secondary
        mClientModeImplListenerCaptor.getValue().onConnectionEnd(secondaryClientModeManager);

        // Telephony coutry is applied after both of them are disconnected.
        verify(mClientModeManager, times(2)).setCountryCode(anyString());
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCodeSentToDriver());
    }

    /**
     * Test if we can reset to the default country code when phone is out of service, when
     * |config_wifi_revert_country_code_on_cellular_loss| is set to true;
     * Telephony service calls |setCountryCode| with an empty string when phone is out of service.
     * In this case we should fall back to the default country code.
     * @throws Exception
     */
    @Test
    public void resetCountryCodeWhenOutOfService() throws Exception {
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCode());
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCode());
        // Out of service.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate("");
        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCode());
    }

    /**
     * Test if we can keep using the last known country code when phone is out of service, when
     * |config_wifi_revert_country_code_on_cellular_loss| is set to false;
     * Telephony service calls |setCountryCode| with an empty string when phone is out of service.
     * In this case we should keep using the last known country code.
     * @throws Exception
     */
    @Test
    public void doNotResetCountryCodeWhenOutOfService() throws Exception {
        // Refresh mWifiCountryCode with |config_wifi_revert_country_code_on_cellular_loss|
        // setting to false.
        mRevertCountryCodeOnCellularLoss = false;
        createWifiCountryCode();

        assertEquals(mDefaultCountryCode, mWifiCountryCode.getCountryCode());
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(mTelephonyCountryCode);
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCode());
        // Out of service.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate("");
        assertEquals(mTelephonyCountryCode, mWifiCountryCode.getCountryCode());
    }

    /**
     * Tests that we always use the US locale for converting the provided country code regardless
     * of the system locale set.
     */
    @Test
    public void useUSLocaleForConversionToUpperCase() {
        String oemCountryCodeLower = "us";
        String oemCountryCodeUpper = "US";
        String telephonyCountryCodeLower = "il";
        String telephonyCountryCodeUpper = "IL";

        mDefaultCountryCode = oemCountryCodeLower;
        createWifiCountryCode();

        // Set the default locale to "tr" (Non US).
        Locale.setDefault(new Locale("tr"));

        // Trigger a country code change using the OEM country code.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        verify(mClientModeManager).setCountryCode(oemCountryCodeUpper);

        // Now trigger a country code change using the telephony country code.
        mWifiCountryCode.setTelephonyCountryCodeAndUpdate(telephonyCountryCodeLower);
        verify(mClientModeManager).setCountryCode(telephonyCountryCodeUpper);
    }
    /**
     * Verifies that dump() does not fail
     */
    @Test
    public void dumpDoesNotFail() {
        StringWriter sw = new StringWriter();
        PrintWriter pw = new PrintWriter(sw);

        mRevertCountryCodeOnCellularLoss = false;
        createWifiCountryCode();

        mWifiCountryCode.dump(null, pw, null);
        String dumpCountryCodeStr = sw.toString();

        assertTrue(dumpCountryCodeStr.contains("mDriverCountryCode"));
        assertTrue(dumpCountryCodeStr.contains("mTelephonyCountryCode"));
        assertTrue(dumpCountryCodeStr.contains("DefaultCountryCode(system property)"));
        assertTrue(dumpCountryCodeStr.contains("DefaultCountryCode(config store)"));
        assertTrue(dumpCountryCodeStr.contains("mTelephonyCountryTimestamp"));
        assertTrue(dumpCountryCodeStr.contains("mDriverCountryTimestamp"));
        assertTrue(dumpCountryCodeStr.contains("mReadyTimestamp"));
        assertTrue(dumpCountryCodeStr.contains("mReady"));
    }

    /**
     * Test set Default country code
     * @throws Exception
     */
    @Test
    public void setDefaultCountryCode() throws Exception {
        // Supplicant started.
        mModeChangeCallbackCaptor.getValue().onActiveModeManagerAdded(mClientModeManager);
        mWifiCountryCode.setDefaultCountryCode(TEST_COUNTRY_CODE);
        verify(mClientModeManager).setCountryCode(eq(TEST_COUNTRY_CODE));
        assertEquals(TEST_COUNTRY_CODE, mWifiCountryCode.getCountryCodeSentToDriver());
        verify(mSettingsConfigStore).put(eq(WIFI_DEFAULT_COUNTRY_CODE), eq(TEST_COUNTRY_CODE));
        assertEquals(TEST_COUNTRY_CODE, mSettingsConfigStore.get(WIFI_DEFAULT_COUNTRY_CODE));
    }

    /**
     * Test is valid country code
     * @throws Exception
     */
    @Test
    public void testValidCountryCode() throws Exception {
        assertEquals(WifiCountryCode.isValid(null), false);
        assertEquals(WifiCountryCode.isValid("JPUS"), false);
        assertEquals(WifiCountryCode.isValid("JP"), true);
        assertEquals(WifiCountryCode.isValid("00"), true);
        assertEquals(WifiCountryCode.isValid("0U"), true);
    }
}
