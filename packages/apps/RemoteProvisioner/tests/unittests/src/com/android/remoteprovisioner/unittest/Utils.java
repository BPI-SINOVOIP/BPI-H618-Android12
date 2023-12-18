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

import com.google.crypto.tink.subtle.Ed25519Sign;

import co.nstant.in.cbor.CborBuilder;
import co.nstant.in.cbor.CborEncoder;
import co.nstant.in.cbor.model.Array;

import org.bouncycastle.asn1.x509.BasicConstraints;
import org.bouncycastle.asn1.x509.Extension;
import org.bouncycastle.asn1.x509.KeyUsage;
import org.bouncycastle.x509.X509V3CertificateGenerator;

import java.io.ByteArrayOutputStream;
import java.math.BigInteger;
import java.security.AlgorithmParameters;
import java.security.KeyFactory;
import java.security.KeyPair;
import java.security.KeyPairGenerator;
import java.security.MessageDigest;
import java.security.PublicKey;
import java.security.cert.X509Certificate;
import java.security.spec.ECGenParameterSpec;
import java.security.spec.ECParameterSpec;
import java.security.spec.ECPoint;
import java.security.spec.ECPublicKeySpec;
import java.time.Duration;
import java.time.Instant;
import java.util.Date;

import javax.security.auth.x500.X500Principal;

/**
 * Utility class for unit testing.
 */
public class Utils {
    private static final int KEY_TYPE = 1;
    private static final int KEY_TYPE_OKP = 1;
    private static final int KID = 2;
    private static final int ALGORITHM = 3;
    private static final int ALGORITHM_EDDSA = -8;
    private static final int ALGORITHM_ECDH_ES_HKDF_256 = -25;
    private static final int CURVE = -1;
    private static final int CURVE_X25519 = 4;
    private static final int CURVE_ED25519 = 6;
    private static final int X_COORDINATE = -2;

    public static PublicKey getP256PubKeyFromBytes(byte[] xPub, byte[] yPub) throws Exception {
        BigInteger x = new BigInteger(1, xPub);
        BigInteger y = new BigInteger(1, yPub);
        AlgorithmParameters parameters = AlgorithmParameters.getInstance("EC");
        parameters.init(new ECGenParameterSpec("secp256r1"));
        ECParameterSpec ecParameters = parameters.getParameterSpec(ECParameterSpec.class);
        ECPoint point = new ECPoint(x, y);
        ECPublicKeySpec keySpec = new ECPublicKeySpec(point, ecParameters);
        KeyFactory keyFactory = KeyFactory.getInstance("EC");
        return keyFactory.generatePublic(keySpec);
    }

    public static KeyPair generateEcdsaKeyPair() throws Exception {
        KeyPairGenerator generator = KeyPairGenerator.getInstance("EC");
        ECGenParameterSpec params = new ECGenParameterSpec("secp256r1");
        generator.initialize(params);
        return generator.generateKeyPair();
    }

    public static X509Certificate signPublicKey(KeyPair issuerKeyPair, PublicKey publicKeyToSign)
            throws Exception {
        X500Principal issuer = new X500Principal("CN=TEE");
        BigInteger serial = BigInteger.ONE;
        X500Principal subject = new X500Principal("CN=TEE");

        Instant now = Instant.now();
        X509V3CertificateGenerator certificateBuilder = new X509V3CertificateGenerator();
        certificateBuilder.setIssuerDN(issuer);
        certificateBuilder.setSerialNumber(serial);
        certificateBuilder.setNotBefore(Date.from(now));
        certificateBuilder.setNotAfter(Date.from(now.plus(Duration.ofDays(1))));
        certificateBuilder.setSignatureAlgorithm("SHA256WITHECDSA");
        certificateBuilder.setSubjectDN(subject);
        certificateBuilder.setPublicKey(publicKeyToSign);
        certificateBuilder.addExtension(
                Extension.basicConstraints, /*isCritical=*/ true, new BasicConstraints(true));
        certificateBuilder.addExtension(
                Extension.keyUsage, /*isCritical=*/ true, new KeyUsage(KeyUsage.keyCertSign));
        return certificateBuilder.generate(issuerKeyPair.getPrivate());
    }

    public static Array encodeAndSignSign1Ed25519(byte[] encodedPublicKey, byte[] privateKey)
            throws Exception {
        byte[] encodedProtectedHeaders = encodeSimpleMap(1, -8);
        return (Array) (new CborBuilder()
            .addArray()
                .add(encodedProtectedHeaders)      // Protected headers
                .addMap()                          // Empty unprotected Headers
                    .end()
                .add(encodedPublicKey)
                .add(encodeAndSignSigStructure(
                        encodedProtectedHeaders, encodedPublicKey, privateKey))
            .end()
            .build().get(0));
    }

    private static byte[] encodeAndSignSigStructure(
            byte[] protectedHeaders, byte[] payload, byte[] privateKey) throws Exception {
        return encodeAndSignSigStructure(protectedHeaders, null, payload, privateKey);
    }

    private static byte[] encodeAndSignSigStructure(byte[] protectedHeaders, byte[] externalAad,
                                                    byte[] payload, byte[] privateKey)
            throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new CborEncoder(baos).encode(new CborBuilder()
                .addArray()
                    .add("Signature1")                                      // context string
                    .add(protectedHeaders)                                  // protected headers
                    .add(null == externalAad ? new byte[0] : externalAad)   // external aad
                    .add(payload)                                           // payload
                    .end()
                .build());
        Ed25519Sign signer = new Ed25519Sign(privateKey);
        return signer.sign(baos.toByteArray());
    }

    public static byte[] encodeEd25519PubKey(byte[] publicKey) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new CborEncoder(baos).encode(new CborBuilder()
                .addMap()
                    .put(KEY_TYPE, KEY_TYPE_OKP)
                    .put(ALGORITHM, ALGORITHM_EDDSA)
                    .put(CURVE, CURVE_ED25519)
                    .put(X_COORDINATE, publicKey)
                    .end()
                .build());
        return baos.toByteArray();
    }

    public static byte[] encodeX25519PubKey(byte[] publicKey) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        byte[] kid = digest.digest(publicKey);
        new CborEncoder(baos).encode(new CborBuilder()
                .addMap()
                    .put(KEY_TYPE, KEY_TYPE_OKP)
                    .put(KID, kid)
                    .put(ALGORITHM, ALGORITHM_ECDH_ES_HKDF_256)
                    .put(CURVE, CURVE_X25519)
                    .put(X_COORDINATE, publicKey)
                    .end()
                .build());
        return baos.toByteArray();
    }

    private static byte[] encodeSimpleMap(int key, int value) throws Exception {
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        new CborEncoder(baos).encode(new CborBuilder()
                .addMap()
                    .put(key, value)
                    .end()
                .build());
        return baos.toByteArray();
    }
}
