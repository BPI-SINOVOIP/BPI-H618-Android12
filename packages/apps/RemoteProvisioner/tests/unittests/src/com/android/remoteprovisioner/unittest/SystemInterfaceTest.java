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

import static android.security.keystore.KeyProperties.KEY_ALGORITHM_EC;
import static android.security.keystore.KeyProperties.PURPOSE_SIGN;

import static com.android.remoteprovisioner.unittest.Utils.generateEcdsaKeyPair;
import static com.android.remoteprovisioner.unittest.Utils.getP256PubKeyFromBytes;
import static com.android.remoteprovisioner.unittest.Utils.signPublicKey;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

import android.hardware.security.keymint.DeviceInfo;
import android.hardware.security.keymint.ProtectedData;
import android.hardware.security.keymint.SecurityLevel;
import android.os.ServiceManager;
import android.platform.test.annotations.Presubmit;
import android.security.keystore.KeyGenParameterSpec;
import android.security.remoteprovisioning.IRemoteProvisioning;

import androidx.test.runner.AndroidJUnit4;

import com.android.remoteprovisioner.CborUtils;
import com.android.remoteprovisioner.SystemInterface;
import com.android.remoteprovisioner.X509Utils;

import com.google.crypto.tink.subtle.Hkdf;
import com.google.crypto.tink.subtle.X25519;

import co.nstant.in.cbor.CborBuilder;
import co.nstant.in.cbor.CborDecoder;
import co.nstant.in.cbor.CborEncoder;
import co.nstant.in.cbor.model.Array;
import co.nstant.in.cbor.model.ByteString;
import co.nstant.in.cbor.model.DataItem;
import co.nstant.in.cbor.model.MajorType;
import co.nstant.in.cbor.model.Map;
import co.nstant.in.cbor.model.NegativeInteger;
import co.nstant.in.cbor.model.UnsignedInteger;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.KeyStore;
import java.security.PublicKey;
import java.security.cert.Certificate;
import java.security.cert.X509Certificate;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import javax.crypto.Cipher;
import javax.crypto.spec.GCMParameterSpec;
import javax.crypto.spec.SecretKeySpec;

@RunWith(AndroidJUnit4.class)
public class SystemInterfaceTest {

    private static final String SERVICE = "android.security.remoteprovisioning";

    private IRemoteProvisioning mBinder;

    @Before
    public void setUp() throws Exception {
        mBinder =
              IRemoteProvisioning.Stub.asInterface(ServiceManager.getService(SERVICE));
        assertNotNull(mBinder);
        mBinder.deleteAllKeys();
    }

    @After
    public void tearDown() throws Exception {
        mBinder.deleteAllKeys();
    }

    private byte[] generateEekChain(byte[] eek) throws Exception {
        com.google.crypto.tink.subtle.Ed25519Sign.KeyPair kp =
                com.google.crypto.tink.subtle.Ed25519Sign.KeyPair.newKeyPair();
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new CborEncoder(baos).encode(new CborBuilder()
                .addArray()
                    .add(Utils.encodeAndSignSign1Ed25519(
                            Utils.encodeEd25519PubKey(kp.getPublicKey()), kp.getPrivateKey()))
                    .add(Utils.encodeAndSignSign1Ed25519(
                            Utils.encodeX25519PubKey(eek), kp.getPrivateKey()))
                    .end()
                .build());
        return baos.toByteArray();
    }

    @Presubmit
    @Test
    public void testGenerateCSR() throws Exception {
        DeviceInfo deviceInfo = new DeviceInfo();
        ProtectedData encryptedBundle = new ProtectedData();
        byte[] eek = new byte[32];
        new Random().nextBytes(eek);
        byte[] bundle =
            SystemInterface.generateCsr(true /* testMode */, 0 /* numKeys */,
                                        SecurityLevel.TRUSTED_ENVIRONMENT,
                                        generateEekChain(eek),
                                        new byte[] {0x02}, encryptedBundle, deviceInfo, mBinder);
        // encryptedBundle should contain a COSE_Encrypt message
        ByteArrayInputStream bais = new ByteArrayInputStream(encryptedBundle.protectedData);
        List<DataItem> dataItems = new CborDecoder(bais).decode();
        assertEquals(1, dataItems.size());
        assertEquals(MajorType.ARRAY, dataItems.get(0).getMajorType());
        Array encMsg = (Array) dataItems.get(0);
        assertEquals(4, encMsg.getDataItems().size());
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

    @Presubmit
    @Test
    public void testGenerateCSRProvisionAndUseKey() throws Exception {
        DeviceInfo deviceInfo = new DeviceInfo();
        ProtectedData encryptedBundle = new ProtectedData();
        int numKeys = 10;
        byte[] eek = new byte[32];
        new Random().nextBytes(eek);
        for (int i = 0; i < numKeys; i++) {
            mBinder.generateKeyPair(true /* testMode */, SecurityLevel.TRUSTED_ENVIRONMENT);
        }
        byte[] bundle =
            SystemInterface.generateCsr(true /* testMode */, numKeys,
                                        SecurityLevel.TRUSTED_ENVIRONMENT,
                                        generateEekChain(eek),
                                        new byte[] {0x02}, encryptedBundle, deviceInfo, mBinder);
        assertNotNull(bundle);
        // The return value of generateCsr should be a COSE_Mac0 message
        ByteArrayInputStream bais = new ByteArrayInputStream(bundle);
        List<DataItem> dataItems = new CborDecoder(bais).decode();
        assertEquals(1, dataItems.size());
        assertEquals(MajorType.ARRAY, dataItems.get(0).getMajorType());
        Array macMsg = (Array) dataItems.get(0);
        assertEquals(4, macMsg.getDataItems().size());

        // The payload for the COSE_Mac0 should contain the array of public keys
        bais = new ByteArrayInputStream(((ByteString) macMsg.getDataItems().get(2)).getBytes());
        List<DataItem> publicKeysArr = new CborDecoder(bais).decode();
        assertEquals(1, publicKeysArr.size());
        assertEquals(MajorType.ARRAY, publicKeysArr.get(0).getMajorType());
        Array publicKeys = (Array) publicKeysArr.get(0);
        assertEquals(numKeys, publicKeys.getDataItems().size());
        KeyPair rootKeyPair = generateEcdsaKeyPair();
        KeyPair intermediateKeyPair = generateEcdsaKeyPair();
        X509Certificate[][] certChain = new X509Certificate[numKeys][3];
        for (int i = 0; i < numKeys; i++) {
            Map publicKey = (Map) publicKeys.getDataItems().get(i);
            byte[] xPub = ((ByteString) publicKey.get(new NegativeInteger(-2))).getBytes();
            byte[] yPub = ((ByteString) publicKey.get(new NegativeInteger(-3))).getBytes();
            assertEquals(xPub.length, 32);
            assertEquals(yPub.length, 32);
            PublicKey leafKeyToSign = getP256PubKeyFromBytes(xPub, yPub);
            certChain[i][0] = signPublicKey(intermediateKeyPair, leafKeyToSign);
            certChain[i][1] = signPublicKey(rootKeyPair, intermediateKeyPair.getPublic());
            certChain[i][2] = signPublicKey(rootKeyPair, rootKeyPair.getPublic());
            ByteArrayOutputStream os = new ByteArrayOutputStream();
            for (int j = 0; j < certChain[i].length; j++) {
                os.write(certChain[i][j].getEncoded());
            }
            SystemInterface.provisionCertChain(X509Utils.getAndFormatRawPublicKey(certChain[i][0]),
                                               certChain[i][0].getEncoded() /* leafCert */,
                                               os.toByteArray() /* certChain */,
                                               System.currentTimeMillis() + 2000 /* validity */,
                                               SecurityLevel.TRUSTED_ENVIRONMENT,
                                               mBinder);
        }
        // getPoolStatus will clean the key pool before we go to assign a new provisioned key
        mBinder.getPoolStatus(0, SecurityLevel.TRUSTED_ENVIRONMENT);
        Certificate[] provisionedCerts1 = generateKeyStoreKey("alias");
        Certificate[] provisionedCerts2 = generateKeyStoreKey("alias2");
        assertEquals(4, provisionedCerts1.length);
        assertEquals(4, provisionedCerts2.length);
        boolean matched = false;
        for (int i = 0; i < certChain.length; i++) {
            if (Arrays.equals(provisionedCerts1[1].getEncoded(), certChain[i][0].getEncoded())) {
                matched = true;
                assertArrayEquals("Second key: j = 0",
                        provisionedCerts2[1].getEncoded(), certChain[i][0].getEncoded());
                for (int j = 1; j < certChain[i].length; j++) {
                    assertArrayEquals("First key: j = " + j,
                            provisionedCerts1[j + 1].getEncoded(), certChain[i][j].getEncoded());
                    assertArrayEquals("Second key: j = " + j,
                            provisionedCerts2[j + 1].getEncoded(), certChain[i][j].getEncoded());
                }
            }
        }
        assertTrue(matched);
    }

    private static byte[] extractRecipientKey(Array recipients) {
        // Recipients is an Array of recipient Arrays
        Map recipientUnprotectedHeaders = (Map) ((Array) recipients.getDataItems().get(0))
                                                                   .getDataItems().get(1);
        Map recipientKeyMap = (Map) recipientUnprotectedHeaders.get(new NegativeInteger(-1));
        return ((ByteString) recipientKeyMap.get(new NegativeInteger(-2))).getBytes();
    }

    private static byte[] buildKdfContext(byte[] serverPub, byte[] ephemeralPub) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new CborEncoder(baos).encode(new CborBuilder()
                .addArray()
                    .add(3) // AlgorithmID: AES-GCM 256
                    .addArray()
                        .add("client".getBytes("UTF8"))
                        .add(new byte[0])
                        .add(ephemeralPub)
                        .end()
                    .addArray()
                        .add("server".getBytes("UTF8"))
                        .add(new byte[0])
                        .add(serverPub)
                        .end()
                    .addArray()
                        .add(256) // key length
                        .add(new byte[0])
                        .end()
                    .end()
                .build());
        return baos.toByteArray();
    }

    private static byte[] buildEncStructure(byte[] protectedHeaders, byte[] externalAad)
            throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new CborEncoder(baos).encode(new CborBuilder()
                .addArray()
                    .add("Encrypt")
                    .add(protectedHeaders)
                    .add(externalAad)
                    .end()
                .build());
        return baos.toByteArray();
    }

    @Presubmit
    @Test
    public void testDecryptProtectedPayload() throws Exception {
        DeviceInfo deviceInfo = new DeviceInfo();
        ProtectedData encryptedBundle = new ProtectedData();
        int numKeys = 1;
        byte[] eekPriv = X25519.generatePrivateKey();
        byte[] eekPub = X25519.publicFromPrivate(eekPriv);
        mBinder.generateKeyPair(true /* testMode */, SecurityLevel.TRUSTED_ENVIRONMENT);
        byte[] bundle =
            SystemInterface.generateCsr(true /* testMode */, numKeys,
                                        SecurityLevel.TRUSTED_ENVIRONMENT,
                                        generateEekChain(eekPub),
                                        new byte[] {0x02}, encryptedBundle, deviceInfo, mBinder);
        ByteArrayInputStream bais = new ByteArrayInputStream(encryptedBundle.protectedData);
        List<DataItem> dataItems = new CborDecoder(bais).decode();
        // Parse encMsg into components: protected and unprotected headers, payload, and recipient
        List<DataItem> encMsg = ((Array) dataItems.get(0)).getDataItems();
        byte[] protectedHeaders = ((ByteString) encMsg.get(0)).getBytes();
        Map unprotectedHeaders = (Map) encMsg.get(1);
        byte[] encryptedContent = ((ByteString) encMsg.get(2)).getBytes();
        Array recipients = (Array) encMsg.get(3);

        byte[] iv = ((ByteString) unprotectedHeaders.get(new UnsignedInteger(5))).getBytes();
        byte[] ephemeralPub = extractRecipientKey(recipients);
        assertEquals(32, ephemeralPub.length);
        byte[] sharedSecret = X25519.computeSharedSecret(eekPriv, ephemeralPub);
        byte[] context = buildKdfContext(eekPub, ephemeralPub);
        byte[] decryptionKey = Hkdf.computeHkdf("HMACSHA256", sharedSecret, null /* salt */,
                                                context, 32);

        Cipher cipher = Cipher.getInstance("AES/GCM/NoPadding");
        cipher.init(
                Cipher.DECRYPT_MODE,
                new SecretKeySpec(decryptionKey, "AES"),
                new GCMParameterSpec(128 /* iv length */, iv));
        cipher.updateAAD(buildEncStructure(protectedHeaders, new byte[0]));

        byte[] protectedData = cipher.doFinal(encryptedContent);
        bais = new ByteArrayInputStream(protectedData);
        List<DataItem> protectedDataArray = new CborDecoder(bais).decode();
        assertEquals(1, protectedDataArray.size());
        assertEquals(MajorType.ARRAY, protectedDataArray.get(0).getMajorType());
        List<DataItem> protectedDataPayload = ((Array) protectedDataArray.get(0)).getDataItems();
        assertTrue(protectedDataPayload.size() == 2 || protectedDataPayload.size() == 3);
    }
}
