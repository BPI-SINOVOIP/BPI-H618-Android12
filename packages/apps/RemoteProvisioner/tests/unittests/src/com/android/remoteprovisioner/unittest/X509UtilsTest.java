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

import static com.android.remoteprovisioner.unittest.Utils.generateEcdsaKeyPair;
import static com.android.remoteprovisioner.unittest.Utils.getP256PubKeyFromBytes;
import static com.android.remoteprovisioner.unittest.Utils.signPublicKey;

import static org.junit.Assert.assertArrayEquals;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;

import android.platform.test.annotations.Presubmit;

import androidx.test.runner.AndroidJUnit4;

import com.android.remoteprovisioner.X509Utils;

import org.junit.Test;
import org.junit.runner.RunWith;

import java.io.ByteArrayOutputStream;
import java.security.KeyPair;
import java.security.cert.X509Certificate;

@RunWith(AndroidJUnit4.class)
public class X509UtilsTest {

    @Presubmit
    @Test
    public void testFormatX509Certs() throws Exception {
        KeyPair root = generateEcdsaKeyPair();
        KeyPair intermediate = generateEcdsaKeyPair();
        KeyPair leaf = generateEcdsaKeyPair();
        X509Certificate[] certs = new X509Certificate[3];
        certs[2] = signPublicKey(root, root.getPublic());
        certs[1] = signPublicKey(root, intermediate.getPublic());
        certs[0] = signPublicKey(intermediate, leaf.getPublic());
        ByteArrayOutputStream os = new ByteArrayOutputStream();
        for (int i = 0; i < certs.length; i++) {
            os.write(certs[i].getEncoded());
        }
        X509Certificate[] roundTrip = X509Utils.formatX509Certs(os.toByteArray());
        assertEquals(certs.length, roundTrip.length);
        for (int i = 0; i < certs.length; i++) {
            assertArrayEquals("Failed on index " + i,
                    certs[i].getEncoded(), roundTrip[i].getEncoded());
        }
    }

    @Presubmit
    @Test
    public void testGetAndFormatRawPublicKey() throws Exception {
        KeyPair testKey = generateEcdsaKeyPair();
        X509Certificate testCert = signPublicKey(testKey, testKey.getPublic());
        byte[] formattedKey = X509Utils.getAndFormatRawPublicKey(testCert);
        byte[] xPoint = new byte[32];
        byte[] yPoint = new byte[32];
        System.arraycopy(formattedKey, 0 /* offset */, xPoint, 0 /* offset */, 32 /* length */);
        System.arraycopy(formattedKey, 32 /* offset */, yPoint, 0 /* offset */, 32 /* length */);
        assertTrue(testKey.getPublic().equals(getP256PubKeyFromBytes(xPoint, yPoint)));
    }
}
