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

package com.android.remoteprovisioner.unittest;

import static android.hardware.security.keymint.SecurityLevel.TRUSTED_ENVIRONMENT;
import static android.security.keystore.KeyProperties.KEY_ALGORITHM_EC;
import static android.security.keystore.KeyProperties.PURPOSE_SIGN;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.content.Context;
import android.os.ServiceManager;
import android.security.keystore.KeyGenParameterSpec;
import android.security.remoteprovisioning.AttestationPoolStatus;
import android.security.remoteprovisioning.ImplInfo;
import android.security.remoteprovisioning.IRemoteProvisioning;

import androidx.test.core.app.ApplicationProvider;
import androidx.test.runner.AndroidJUnit4;

import com.android.remoteprovisioner.GeekResponse;
import com.android.remoteprovisioner.Provisioner;
import com.android.remoteprovisioner.ServerInterface;
import com.android.remoteprovisioner.SettingsManager;

import org.junit.After;
import org.junit.Before;
import org.junit.BeforeClass;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.time.Duration;
import java.util.Arrays;

@RunWith(AndroidJUnit4.class)
public class ServerToSystemTest {

    private static final boolean IS_TEST_MODE = false;
    private static final String SERVICE = "android.security.remoteprovisioning";

    private static Context sContext;
    private static IRemoteProvisioning sBinder;
    private static int sCurve = 0;

    private Duration mDuration;

    private void assertPoolStatus(int total, int attested,
                                  int unassigned, int expiring, Duration time) throws Exception {
        AttestationPoolStatus pool = sBinder.getPoolStatus(time.toMillis(), TRUSTED_ENVIRONMENT);
        assertEquals(total, pool.total);
        assertEquals(attested, pool.attested);
        assertEquals(unassigned, pool.unassigned);
        assertEquals(expiring, pool.expiring);
    }

    private static Certificate[] generateKeyStoreKey(String alias) throws Exception {
        KeyStore keyStore = KeyStore.getInstance("AndroidKeyStore");
        keyStore.load(null);
        KeyPairGenerator keyPairGenerator = KeyPairGenerator.getInstance(KEY_ALGORITHM_EC,
                "AndroidKeyStore");
        KeyGenParameterSpec spec = new KeyGenParameterSpec.Builder(alias, PURPOSE_SIGN)
                .setAttestationChallenge("challenge".getBytes())
                .build();
        keyPairGenerator.initialize(spec);
        keyPairGenerator.generateKeyPair();
        Certificate[] certs = keyStore.getCertificateChain(spec.getKeystoreAlias());
        keyStore.deleteEntry(alias);
        return certs;
    }

    @BeforeClass
    public static void init() throws Exception {
        sContext = ApplicationProvider.getApplicationContext();
        sBinder =
              IRemoteProvisioning.Stub.asInterface(ServiceManager.getService(SERVICE));
        assertNotNull(sBinder);
        ImplInfo[] info = sBinder.getImplementationInfo();
        for (int i = 0; i < info.length; i++) {
            if (info[i].secLevel == TRUSTED_ENVIRONMENT) {
                sCurve = info[i].supportedCurve;
                break;
            }
        }
    }

    @Before
    public void setUp() throws Exception {
        SettingsManager.clearPreferences(sContext);
        sBinder.deleteAllKeys();
        mDuration = Duration.ofMillis(System.currentTimeMillis());
    }

    @After
    public void tearDown() throws Exception {
        SettingsManager.clearPreferences(sContext);
        sBinder.deleteAllKeys();
    }

    @Test
    public void testFullRoundTrip() throws Exception {
        int numTestKeys = 1;
        assertPoolStatus(0, 0, 0, 0, mDuration);
        sBinder.generateKeyPair(IS_TEST_MODE, TRUSTED_ENVIRONMENT);
        assertPoolStatus(numTestKeys, 0, 0, 0, mDuration);
        GeekResponse geek = ServerInterface.fetchGeek(sContext);
        assertNotNull(geek);
        int numProvisioned =
                Provisioner.provisionCerts(numTestKeys, TRUSTED_ENVIRONMENT,
                                           geek.getGeekChain(sCurve), geek.getChallenge(), sBinder,
                                           sContext);
        assertEquals(numTestKeys, numProvisioned);
        assertPoolStatus(numTestKeys, numTestKeys, numTestKeys, 0, mDuration);
        // Certificate duration sent back from the server may change, however ~6 months should be
        // pretty safe.
        assertPoolStatus(numTestKeys, numTestKeys, numTestKeys,
                         numTestKeys, mDuration.plusDays(180));
    }

    @Test
    public void testFallback() throws Exception {
        // Feed a fake URL into the device config to ensure that remote provisioning fails.
        SettingsManager.setDeviceConfig(sContext, 2 /* extraKeys */, mDuration /* expiringBy */,
                                        "Not even a URL" /* url */);
        int numTestKeys = 1;
        assertPoolStatus(0, 0, 0, 0, mDuration);
        Certificate[] fallbackKeyCerts1 = generateKeyStoreKey("test1");

        SettingsManager.clearPreferences(sContext);
        sBinder.generateKeyPair(IS_TEST_MODE, TRUSTED_ENVIRONMENT);
        GeekResponse geek = ServerInterface.fetchGeek(sContext);
        int numProvisioned =
                Provisioner.provisionCerts(numTestKeys, TRUSTED_ENVIRONMENT,
                                           geek.getGeekChain(sCurve), geek.getChallenge(), sBinder,
                                           sContext);
        assertEquals(numTestKeys, numProvisioned);
        assertPoolStatus(numTestKeys, numTestKeys, numTestKeys, 0, mDuration);
        Certificate[] provisionedKeyCerts = generateKeyStoreKey("test2");
        sBinder.deleteAllKeys();
        sBinder.generateKeyPair(IS_TEST_MODE, TRUSTED_ENVIRONMENT);

        SettingsManager.setDeviceConfig(sContext, 2 /* extraKeys */, mDuration /* expiringBy */,
                                        "Not even a URL" /* url */);
        // Even if there is an unsigned key hanging around, fallback should still occur.
        Certificate[] fallbackKeyCerts2 = generateKeyStoreKey("test3");
        // Due to there being no attested keys in the pool, the provisioning service should not
        // have even attempted to provision more certificates.
        assertEquals(0, SettingsManager.getFailureCounter(sContext));
        assertTrue(fallbackKeyCerts1.length == fallbackKeyCerts2.length);
        for (int i = 1; i < fallbackKeyCerts1.length; i++) {
            assertArrayEquals("Cert: " + i, fallbackKeyCerts1[i].getEncoded(),
                              fallbackKeyCerts2[i].getEncoded());
        }
        assertTrue(provisionedKeyCerts.length > 0);
        // The root certificates should not match.
        assertFalse("Provisioned and fallback attestation key root certificates match.",
                    Arrays.equals(fallbackKeyCerts1[fallbackKeyCerts1.length - 1].getEncoded(),
                              provisionedKeyCerts[provisionedKeyCerts.length - 1].getEncoded()));
    }
}
