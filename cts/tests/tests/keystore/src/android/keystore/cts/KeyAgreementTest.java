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

import java.security.GeneralSecurityException;
import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.NoSuchAlgorithmException;
import java.security.NoSuchProviderException;
import java.security.spec.InvalidKeySpecException;

import javax.crypto.KeyAgreement;

import junit.framework.TestCase;

import org.junit.Assert;

import android.content.Context;
import android.security.keystore.KeyGenParameterSpec;
import android.security.keystore.KeyProperties;
import android.security.keystore.KeyInfo;
import androidx.test.InstrumentationRegistry;

public class KeyAgreementTest extends TestCase {
    private static final String PRIVATE_KEY_ALIAS = "TemporaryPrivateKey";

    public void testGenerateSecret_succeeds() throws Exception {
        KeyAgreement ka = getKeyStoreKeyAgreement();
        ka.init(generateEphemeralAndroidKeyPair(PRIVATE_KEY_ALIAS).getPrivate());
        ka.doPhase(generateEphemeralServerKeyPair().getPublic(), true);
        byte[] sharedSecret = ka.generateSecret();
        assertNotNull(sharedSecret);
    }

    public void testGenerateSecret_forTwoParties_returnsSameSharedSecret() throws Exception {
        KeyPair ourKeyPair = generateEphemeralAndroidKeyPair(PRIVATE_KEY_ALIAS);
        KeyPair theirKeyPair = generateEphemeralServerKeyPair();

        KeyAgreement ka = getKeyStoreKeyAgreement();
        // Generate the shared secret with our own private key and the public key of the other party
        ka.init(ourKeyPair.getPrivate());
        ka.doPhase(theirKeyPair.getPublic(), true);
        byte[] ourSharedSecret = ka.generateSecret();
        // Generate the shared secret as if we were the other party
        KeyAgreement otherKeyAgreement = KeyAgreement.getInstance("ECDH"); // Use default provider
        otherKeyAgreement.init(theirKeyPair.getPrivate());
        otherKeyAgreement.doPhase(ourKeyPair.getPublic(), true);
        byte[] theirSharedSecret = otherKeyAgreement.generateSecret();

        Assert.assertArrayEquals(ourSharedSecret, theirSharedSecret);
    }

    public void testGenerateSecret_preservesPrivateKeyAndNothingElse() throws Exception {
        KeyPair otherPartyKey = generateEphemeralServerKeyPair();
        KeyAgreement ka = getKeyStoreKeyAgreement();
        ka.init(generateEphemeralAndroidKeyPair(PRIVATE_KEY_ALIAS).getPrivate());
        ka.doPhase(otherPartyKey.getPublic(), true);
        byte[] sharedSecret1 = ka.generateSecret();

        try {
            ka.generateSecret();
            fail(
                    "Should not be able to generate secret twice. Only private key should have"
                            + " been retained");
        } catch (IllegalStateException ise) {
            // Expected
        }
        ka.doPhase(otherPartyKey.getPublic(), true);
        byte[] sharedSecret2 = ka.generateSecret();

        Assert.assertArrayEquals(sharedSecret1, sharedSecret2);
    }

    public void testInit_withNonPrivateKey_fails() throws Exception {
        KeyAgreement ka = getKeyStoreKeyAgreement();
        try {
            ka.init(new TransparentSecretKey(new byte[0], null));
            fail("Initializing KeyAgreement with non-keystore key didn't throw exception.");
        } catch (InvalidKeyException ike) {
            // Expected
        }
    }

    public void testInit_withNonEcKey_fails() throws Exception {
        KeyPairGenerator kpg =
                KeyPairGenerator.getInstance(KeyProperties.KEY_ALGORITHM_RSA, "AndroidKeyStore");
        kpg.initialize(
                new KeyGenParameterSpec.Builder("rsakey", KeyProperties.PURPOSE_AGREE_KEY).build());
        KeyPair rsaKeyPair = kpg.genKeyPair();
        KeyAgreement ka = getKeyStoreKeyAgreement();

        try {
            ka.init(rsaKeyPair.getPrivate());
            fail("Initializing KeyAgreement with non-EC key should fail.");
        } catch (InvalidKeyException ike) {
            // Expected
        }
    }

    public void testDoPhase_withoutInitialization_fails() throws Exception {
        KeyAgreement ka = getKeyStoreKeyAgreement();
        try {
            ka.doPhase(generateEphemeralServerKeyPair().getPublic(), true);
            fail("Should not be able to call doPhase without performing initialization first");
        } catch (IllegalStateException ise) {
            // Expected
        }
    }

    public void testGenerateSecret_withoutSecondPartyKey_fails() throws Exception {
        KeyAgreement ka = getKeyStoreKeyAgreement();
        ka.init(generateEphemeralAndroidKeyPair(PRIVATE_KEY_ALIAS).getPrivate());
        try {
            ka.generateSecret();
            fail("Should not be able to generate secret without other party key.");
        } catch (IllegalStateException ise) {
            // Expected
        }
    }

    public void testDoPhase_multiparty_fails() throws Exception {
        // Multi-party key agreement is not supported by Keymint
        KeyAgreement ka = getKeyStoreKeyAgreement();
        ka.init(generateEphemeralAndroidKeyPair(PRIVATE_KEY_ALIAS).getPrivate());
        try {
            ka.doPhase(generateEphemeralServerKeyPair().getPublic(), false);
            fail("Calling doPhase with lastPhase set to false should fail.");
        } catch (IllegalStateException ise) {
            // Expected
        }
        ka.doPhase(generateEphemeralServerKeyPair().getPublic(), true);
        try {
            ka.doPhase(generateEphemeralServerKeyPair().getPublic(), true);
            fail("Calling doPhase multiple times should fail.");
        } catch (IllegalStateException ise) {
            // Expected
        }
    }

    private static KeyPair generateEphemeralAndroidKeyPair(String alias)
            throws GeneralSecurityException {
        KeyPairGenerator kpg =
                KeyPairGenerator.getInstance(KeyProperties.KEY_ALGORITHM_EC, "AndroidKeyStore");
        kpg.initialize(
                new KeyGenParameterSpec.Builder(alias, KeyProperties.PURPOSE_AGREE_KEY).build());

        KeyPair kp = kpg.generateKeyPair();

        KeyFactory factory = KeyFactory.getInstance(kp.getPrivate().getAlgorithm(),
                "AndroidKeyStore");
        KeyInfo keyInfo = null;
        try {
            keyInfo = factory.getKeySpec(kp.getPrivate(), KeyInfo.class);
        } catch (InvalidKeySpecException e) {
            // Not an Android KeyStore key.
            fail("Unable to get KeyInfo for created key.");
        }

        // ECDH is only implemented in Secure Hardware if KeyMint is available.
        int level = keyInfo.getSecurityLevel();
        Context context = InstrumentationRegistry.getTargetContext();
        if (TestUtils.getFeatureVersionKeystore(context) >= Attestation.KM_VERSION_KEYMINT_1) {
            Assert.assertTrue(
                level == KeyProperties.SECURITY_LEVEL_TRUSTED_ENVIRONMENT ||
                level == KeyProperties.SECURITY_LEVEL_UNKNOWN_SECURE);
        } else {
            Assert.assertEquals(keyInfo.getSecurityLevel(),
                    KeyProperties.SECURITY_LEVEL_SOFTWARE);
        }

        return kp;
    }

    private static KeyPair generateEphemeralServerKeyPair() throws GeneralSecurityException {
        KeyPairGenerator kpg = KeyPairGenerator.getInstance("EC"); // Use default provider
        kpg.initialize(256);

        return kpg.generateKeyPair();
    }

    // Helper function allowing us to include information about Keystore2 likely not being available
    // in the test output.
    private static KeyAgreement getKeyStoreKeyAgreement() throws NoSuchProviderException {
        try {
            KeyAgreement ka = KeyAgreement.getInstance("ECDH", "AndroidKeyStore");
            return ka;
        } catch (NoSuchAlgorithmException nsa) {
            fail(
                    "AndroidKeyStore is missing an ECDH implementation, which is likely caused by"
                        + " keystore2 not being enabled. See b/160623310 for more information.");
            return null;
        }
    }
}
