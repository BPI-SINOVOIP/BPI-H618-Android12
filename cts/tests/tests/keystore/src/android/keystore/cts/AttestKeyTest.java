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

package android.keystore.cts;

import static android.keystore.cts.KeyAttestationTest.verifyCertificateChain;
import static android.security.keystore.KeyProperties.KEY_ALGORITHM_EC;
import static android.security.keystore.KeyProperties.PURPOSE_ATTEST_KEY;
import static android.security.keystore.KeyProperties.PURPOSE_SIGN;

import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.MatcherAssert.assertThat;
import static org.hamcrest.Matchers.greaterThan;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.Assume.assumeTrue;

import android.content.pm.PackageManager;
import android.security.keystore.KeyGenParameterSpec;
import android.util.Log;

import androidx.test.platform.app.InstrumentationRegistry;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;

import java.security.GeneralSecurityException;
import java.security.InvalidAlgorithmParameterException;
import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.KeyStoreException;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.stream.Stream;

import javax.security.auth.x500.X500Principal;

public class AttestKeyTest {
    private static final String TAG = AttestKeyTest.class.getSimpleName();

    private KeyStore mKeyStore;
    private ArrayList<String> mAliasesToDelete = new ArrayList();

    @Before
    public void setUp() throws Exception {
        mKeyStore = KeyStore.getInstance("AndroidKeyStore");
        mKeyStore.load(null);

        // Assume attest key support for all tests in this class.
        assumeAttestKey();
    }

    @After
    public void tearDown() throws Exception {
        for (String alias : mAliasesToDelete) {
            try {
                mKeyStore.deleteEntry(alias);
            } catch (Throwable t) {
                // Ignore any exception and delete the other aliases in the list.
            }
        }
    }

    @Test
    public void testEcAttestKey() throws Exception {
        final String attestKeyAlias = "attestKey";

        Certificate attestKeyCertChain[] = generateKeyPair(KEY_ALGORITHM_EC,
                new KeyGenParameterSpec.Builder(attestKeyAlias, PURPOSE_ATTEST_KEY)
                        .setAttestationChallenge("challenge".getBytes())
                        .build());
        assertThat(attestKeyCertChain.length, greaterThan(1));

        final String attestedKeyAlias = "attestedKey";
        Certificate attestedKeyCertChain[] = generateKeyPair(KEY_ALGORITHM_EC,
                new KeyGenParameterSpec.Builder(attestedKeyAlias, PURPOSE_SIGN)
                        .setAttestationChallenge("challenge".getBytes())
                        .setAttestKeyAlias(attestKeyAlias)
                        .build());
        // Even though we asked for an attestation, we only get one cert.
        assertThat(attestedKeyCertChain.length, is(1));

        verifyCombinedChain(attestKeyCertChain, attestedKeyCertChain);

        X509Certificate attestationCert = (X509Certificate) attestedKeyCertChain[0];
        Attestation attestation = Attestation.loadFromCertificate(attestationCert);
    }

    @Test
    public void testAttestationWithNonAttestKey() throws Exception {
        final String nonAttestKeyAlias = "nonAttestKey";
        final String attestedKeyAlias = "attestedKey";
        generateKeyPair(KEY_ALGORITHM_EC,
                new KeyGenParameterSpec.Builder(nonAttestKeyAlias, PURPOSE_SIGN).build());

        try {
            generateKeyPair(KEY_ALGORITHM_EC,
                    new KeyGenParameterSpec.Builder(attestedKeyAlias, PURPOSE_SIGN)
                            .setAttestationChallenge("challenge".getBytes())
                            .setAttestKeyAlias(nonAttestKeyAlias)
                            .build());
            fail("Expected exception.");
        } catch (InvalidAlgorithmParameterException e) {
            assertThat(e.getMessage(), is("Invalid attestKey, does not have PURPOSE_ATTEST_KEY"));
        }
    }

    @Test
    public void testAttestKeyWithoutChallenge() throws Exception {
        final String attestKeyAlias = "attestKey";
        final String attestedKeyAlias = "attestedKey";
        generateKeyPair(KEY_ALGORITHM_EC,
                new KeyGenParameterSpec.Builder(attestKeyAlias, PURPOSE_ATTEST_KEY).build());

        try {
            generateKeyPair(KEY_ALGORITHM_EC,
                    new KeyGenParameterSpec.Builder(attestedKeyAlias, PURPOSE_SIGN)
                            // Don't set attestation challenge
                            .setAttestKeyAlias(attestKeyAlias)
                            .build());
            fail("Expected exception.");
        } catch (InvalidAlgorithmParameterException e) {
            assertThat(e.getMessage(),
                    is("AttestKey specified but no attestation challenge provided"));
        }
    }

    @Test
    public void testAttestKeySecurityLevelMismatch() throws Exception {
        assumeStrongBox();

        final String strongBoxAttestKeyAlias = "nonAttestKey";
        final String attestedKeyAlias = "attestedKey";
        generateKeyPair(KEY_ALGORITHM_EC,
                new KeyGenParameterSpec.Builder(strongBoxAttestKeyAlias,
                        PURPOSE_ATTEST_KEY).setIsStrongBoxBacked(true).build());

        try {
            generateKeyPair(KEY_ALGORITHM_EC,
                    new KeyGenParameterSpec.Builder(attestedKeyAlias, PURPOSE_SIGN)
                            .setAttestationChallenge("challenge".getBytes())
                            .setAttestKeyAlias(strongBoxAttestKeyAlias)
                            .build());
            fail("Expected exception.");
        } catch (InvalidAlgorithmParameterException e) {
            assertThat(e.getMessage(),
                    is("Invalid security level: Cannot sign non-StrongBox key with StrongBox "
                            + "attestKey"));
        }
    }

    private void assumeStrongBox() {
        PackageManager packageManager =
                InstrumentationRegistry.getInstrumentation().getTargetContext().getPackageManager();
        assumeTrue("Can only test if we have StrongBox",
                packageManager.hasSystemFeature(PackageManager.FEATURE_STRONGBOX_KEYSTORE));
    }

    private void assumeAttestKey() {
        PackageManager packageManager =
                InstrumentationRegistry.getInstrumentation().getTargetContext().getPackageManager();
        assumeTrue("Can only test if we have the attest key feature.",
                packageManager.hasSystemFeature(PackageManager.FEATURE_KEYSTORE_APP_ATTEST_KEY));
    }

    private Certificate[] generateKeyPair(String algorithm, KeyGenParameterSpec spec)
            throws NoSuchAlgorithmException, NoSuchProviderException,
            InvalidAlgorithmParameterException, KeyStoreException {
        KeyPairGenerator keyPairGenerator = KeyPairGenerator.getInstance(algorithm,
                "AndroidKeyStore");
        keyPairGenerator.initialize(spec);
        keyPairGenerator.generateKeyPair();
        mAliasesToDelete.add(spec.getKeystoreAlias());

        return mKeyStore.getCertificateChain(spec.getKeystoreAlias());
    }

    private void verifyCombinedChain(Certificate[] attestKeyCertChain,
            Certificate[] attestedKeyCertChain) throws GeneralSecurityException {
        Certificate[] combinedChain = Stream
                .concat(Arrays.stream(attestedKeyCertChain),
                        Arrays.stream(attestKeyCertChain))
                .toArray(Certificate[]::new);

        int i = 0;
        for (Certificate cert : combinedChain) {
            Log.e(TAG, "Certificate " + i + ": " + cert);
            ++i;
        }

        verifyCertificateChain((Certificate[]) combinedChain, false /* expectStrongBox */);
    }
}
