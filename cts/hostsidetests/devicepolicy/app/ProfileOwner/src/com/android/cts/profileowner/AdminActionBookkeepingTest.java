/*
 * Copyright (C) 2017 The Android Open Source Project
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
package com.android.cts.profileowner;

import android.content.ContentResolver;
import android.os.Process;
import android.provider.Settings;

import com.android.cts.devicepolicy.TestCertificates;

import java.io.ByteArrayInputStream;
import java.security.KeyStore;
import java.security.cert.Certificate;
import java.security.cert.CertificateFactory;
import java.util.List;

public class AdminActionBookkeepingTest extends BaseProfileOwnerTest {
    @Override
    protected void tearDown() throws Exception {
        mDevicePolicyManager.uninstallCaCert(getWho(), TestCertificates.TEST_CA.getBytes());

        super.tearDown();
    }

    /**
     * Test: It should be recored whether the Profile Owner or the user set the current IME.
     */
    public void testIsDefaultInputMethodSet() throws Exception {
        final String setting = Settings.Secure.DEFAULT_INPUT_METHOD;
        final ContentResolver resolver = getContext().getContentResolver();
        final String ime = Settings.Secure.getString(resolver, setting);

        Settings.Secure.putString(resolver, setting, "com.test.1");
        Thread.sleep(500);
        assertFalse(mDevicePolicyManager.isCurrentInputMethodSetByOwner());

        mDevicePolicyManager.setSecureSetting(getWho(), setting, "com.test.2");
        Thread.sleep(500);
        assertTrue(mDevicePolicyManager.isCurrentInputMethodSetByOwner());

        Settings.Secure.putString(resolver, setting, ime);
        Thread.sleep(500);
        assertFalse(mDevicePolicyManager.isCurrentInputMethodSetByOwner());
    }

    /**
     * Test: It should be recored whether the Profile Owner or the user installed a CA cert.
     */
    public void testGetPolicyInstalledCaCerts() throws Exception {
        final byte[] rawCert = TestCertificates.TEST_CA.getBytes();
        final Certificate cert = CertificateFactory.getInstance("X.509")
                .generateCertificate(new ByteArrayInputStream(rawCert));

        // Install a CA cert.
        KeyStore keyStore = KeyStore.getInstance("AndroidCAStore");
        keyStore.load(null, null);
        assertNull(keyStore.getCertificateAlias(cert));
        assertTrue(mDevicePolicyManager.installCaCert(getWho(), rawCert));
        final String alias = keyStore.getCertificateAlias(cert);
        assertNotNull(alias);

        // Verify that the CA cert was marked as installed by the Profile Owner.
        verifyOwnerInstalledStatus(alias, true);

        // Uninstall the CA cert.
        mDevicePolicyManager.uninstallCaCert(getWho(), rawCert);

        // Verify that the CA cert is no longer marked as installed by the Profile Owner.
        verifyOwnerInstalledStatus(alias, false);
    }

    private void verifyOwnerInstalledStatus(String alias, boolean expectOwnerInstalled) {
        final List<String> ownerInstalledCerts =
                mDevicePolicyManager.getOwnerInstalledCaCerts(Process.myUserHandle());
        assertNotNull(ownerInstalledCerts);
        assertEquals(expectOwnerInstalled, ownerInstalledCerts.contains(alias));
    }
}
