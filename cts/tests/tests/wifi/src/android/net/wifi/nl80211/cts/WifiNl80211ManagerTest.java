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

package android.net.wifi.nl80211.cts;

import static android.net.wifi.nl80211.WifiNl80211Manager.OemSecurityType;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertFalse;
import static org.junit.Assume.assumeTrue;

import android.content.Context;
import android.net.wifi.ScanResult;
import android.net.wifi.WifiManager;
import android.net.wifi.cts.WifiFeature;
import android.net.wifi.nl80211.WifiNl80211Manager;
import android.os.Build;
import android.platform.test.annotations.AppModeFull;

import androidx.test.ext.junit.runners.AndroidJUnit4;
import androidx.test.filters.SdkSuppress;
import androidx.test.filters.SmallTest;
import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.util.Arrays;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.Executor;


/** CTS tests for {@link WifiNl80211Manager}. */
@SmallTest
@RunWith(AndroidJUnit4.class)
@AppModeFull(reason = "Cannot get WifiManager/WifiNl80211Manager in instant app mode")
public class WifiNl80211ManagerTest {

    private Context mContext;

    private static class TestExecutor implements Executor {
        private ConcurrentLinkedQueue<Runnable> tasks = new ConcurrentLinkedQueue<>();

        @Override
        public void execute(Runnable task) {
            tasks.add(task);
        }

        private void runAll() {
            Runnable task = tasks.poll();
            while (task != null) {
                task.run();
                task = tasks.poll();
            }
        }
    }

    private class TestCountryCodeChangeListener implements
            WifiNl80211Manager.CountryCodeChangedListener {
        private String mCurrentCountryCode;

        public String getCurrentCountryCode() {
            return mCurrentCountryCode;
        }

        @Override
        public void onCountryCodeChanged(String country) {
            mCurrentCountryCode = country;
        }
    }

    @Before
    public void setUp() {
        mContext = InstrumentationRegistry.getInstrumentation().getContext();
        // skip tests if Wifi is not supported
        assumeTrue(WifiFeature.isWifiSupported(mContext));
    }

    @Test
    public void testOemSecurityTypeConstructor() {
        OemSecurityType securityType = new OemSecurityType(
                ScanResult.PROTOCOL_WPA,
                Arrays.asList(ScanResult.KEY_MGMT_PSK, ScanResult.KEY_MGMT_SAE),
                Arrays.asList(ScanResult.CIPHER_NONE, ScanResult.CIPHER_TKIP),
                ScanResult.CIPHER_CCMP);

        assertThat(securityType.protocol).isEqualTo(ScanResult.PROTOCOL_WPA);
        assertThat(securityType.keyManagement)
                .isEqualTo(Arrays.asList(ScanResult.KEY_MGMT_PSK, ScanResult.KEY_MGMT_SAE));
        assertThat(securityType.pairwiseCipher)
                .isEqualTo(Arrays.asList(ScanResult.CIPHER_NONE, ScanResult.CIPHER_TKIP));
        assertThat(securityType.groupCipher).isEqualTo(ScanResult.CIPHER_CCMP);
    }

    @Test
    public void testSendMgmtFrame() {
        try {
            WifiNl80211Manager manager = mContext.getSystemService(WifiNl80211Manager.class);
            manager.sendMgmtFrame("wlan0", new byte[]{}, -1, Runnable::run,
                    new WifiNl80211Manager.SendMgmtFrameCallback() {
                        @Override
                        public void onAck(int elapsedTimeMs) {}

                        @Override
                        public void onFailure(int reason) {}
                    });
        } catch (Exception ignore) {}
    }

    @Test
    public void testGetTxPacketCounters() {
        try {
            WifiNl80211Manager manager = mContext.getSystemService(WifiNl80211Manager.class);
            manager.getTxPacketCounters("wlan0");
        } catch (Exception ignore) {}
    }

    @Test
    public void testSetOnServiceDeadCallback() {
        try {
            WifiNl80211Manager manager = mContext.getSystemService(WifiNl80211Manager.class);
            manager.setOnServiceDeadCallback(() -> {});
        } catch (Exception ignore) {}
    }

    @SdkSuppress(minSdkVersion = Build.VERSION_CODES.S)
    @Test
    public void testCountryCodeChangeListener() {
        TestCountryCodeChangeListener testCountryCodeChangeListener =
                new TestCountryCodeChangeListener();
        TestExecutor executor = new TestExecutor();
        WifiManager wifiManager = mContext.getSystemService(WifiManager.class);
        // Enable wifi to trigger country code change
        wifiManager.setWifiEnabled(true);
        WifiNl80211Manager manager = mContext.getSystemService(WifiNl80211Manager.class);
        // Register listener and unregister listener for API coverage only.
        // Since current cts don't have sufficient permission to call WifiNl80211Manager API.
        // Assert register fail because the CTS don't have sufficient permission to call
        // WifiNl80211Manager API which are guarded by selinux.
        assertFalse(manager.registerCountryCodeChangedListener(executor,
                testCountryCodeChangeListener));
        manager.unregisterCountryCodeChangedListener(testCountryCodeChangeListener);
    }
}
