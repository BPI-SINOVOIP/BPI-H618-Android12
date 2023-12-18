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
package com.android.cts.delegate;

import static android.app.admin.DevicePolicyManager.DELEGATION_CERT_INSTALL;

import static com.android.cts.delegate.DelegateTestUtils.assertExpectException;
import static com.android.cts.devicepolicy.TestCertificates.TEST_CA;
import static com.android.cts.devicepolicy.TestCertificates.TEST_CERT;
import static com.android.cts.devicepolicy.TestCertificates.TEST_KEY;

import android.app.admin.DevicePolicyManager;
import android.content.ComponentName;
import android.util.Base64;
import android.util.Base64InputStream;

import com.android.cts.devicepolicy.ParcelablePrivateKey;

import java.io.ByteArrayInputStream;
import java.security.PrivateKey;
import java.security.cert.Certificate;
import java.security.cert.CertificateException;
import java.security.cert.CertificateFactory;
import java.util.List;

/**
 * Tests that a package other than the DPC can manage app restrictions if allowed by the DPC
 * via {@link DevicePolicyManager#setApplicationRestrictionsManagingPackage(ComponentName, String)}
 */
public class CertInstallDelegateTest extends BaseJUnit3TestCase {

    public void testCannotAccessApis() {
        assertFalse(amICertInstallDelegate());

        assertExpectException(SecurityException.class,
                "Calling identity is not authorized", () -> {
                    mDpm.installCaCert(null, null);
                });

        assertExpectException(SecurityException.class,
                "Calling identity is not authorized", () -> {
                    mDpm.removeKeyPair(null, "alias");
                });
    }

    public void testCanAccessApis() throws Exception {
        assertTrue(amICertInstallDelegate());

        byte[] cert = TEST_CA.getBytes();

        // Exercise installCaCert.
        assertTrue("Certificate installation failed", mDpm.installCaCert(null, cert));

        // Exercise hasCertInstalled.
        assertTrue("Certificate is not installed properly", mDpm.hasCaCertInstalled(null, cert));

        // Exercise getInstalledCaCerts.
        assertTrue("Certificate is not among installed certs",
                containsCertificate(mDpm.getInstalledCaCerts(null), cert));

        // Exercise uninstallCaCert.
        mDpm.uninstallCaCert(null, cert);
        assertFalse("Certificate was not uninstalled", mDpm.hasCaCertInstalled(null, cert));

        // Exercise installKeyPair.
        final String alias = "delegated-cert-installer-test-key";

        PrivateKey privatekey = new ParcelablePrivateKey("RSA",
                Base64.decode(TEST_KEY, Base64.DEFAULT));

        Certificate certificate = CertificateFactory.getInstance("X.509").generateCertificate(
                new Base64InputStream(new ByteArrayInputStream(TEST_CERT.getBytes()),
                    Base64.DEFAULT));
        assertTrue("Key pair not installed successfully",
                mDpm.installKeyPair(null, privatekey, certificate, alias));

        // Exercise removeKeyPair.
        assertTrue("Key pair not removed successfully", mDpm.removeKeyPair(null, alias));
    }

    private static boolean containsCertificate(List<byte[]> certificates, byte[] toMatch)
            throws CertificateException {
        Certificate certificateToMatch = readCertificate(toMatch);
        for (byte[] certBuffer : certificates) {
            Certificate cert = readCertificate(certBuffer);
            if (certificateToMatch.equals(cert)) {
                return true;
            }
        }
        return false;
    }

    private static Certificate readCertificate(byte[] certBuffer) throws CertificateException {
        final CertificateFactory certFactory = CertificateFactory.getInstance("X.509");
        return certFactory.generateCertificate(new ByteArrayInputStream(certBuffer));
    }

    private boolean amICertInstallDelegate() {
        final String packageName = getInstrumentation().getContext().getPackageName();
        final List<String> scopes = mDpm.getDelegatedScopes(null, packageName);
        return scopes.contains(DELEGATION_CERT_INSTALL);
    }
}
