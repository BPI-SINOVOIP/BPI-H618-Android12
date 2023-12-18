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

package com.android.cts.deviceandprofileowner;

import static com.google.common.truth.Truth.assertThat;

import static org.testng.Assert.assertThrows;

import android.annotation.NonNull;
import android.app.UiAutomation;
import android.net.wifi.WifiManager;
import android.os.Build;
import android.telephony.TelephonyManager;

import java.nio.ByteBuffer;
import java.security.InvalidKeyException;
import java.security.NoSuchAlgorithmException;

import javax.crypto.Mac;
import javax.crypto.spec.SecretKeySpec;

/**
 * Tests for the Enrollment-Specific ID functionality.
 *
 * NOTE: Tests in this class need to be run separately from the host-side since each
 * sets a non-resettable Organization ID, so the DPC needs to be completely removed
 * before each test.
 */
public class EnrollmentSpecificIdTest extends BaseDeviceAdminTest {
    private static final String[] PERMISSIONS_TO_ADOPT = {
            "android.permission.READ_PRIVILEGED_PHONE_STATE",
            "android.permission.NETWORK_SETTINGS",
            "android.permission.LOCAL_MAC_ADDRESS"};
    private static final String ORGANIZATION_ID = "abcxyz123";
    private UiAutomation mUiAutomation;

    @Override
    protected void setUp() throws Exception {
        super.setUp();
        mUiAutomation = getInstrumentation().getUiAutomation();
    }

    public void testThrowsForEmptyOrganizationId() {
        assertThrows(IllegalArgumentException.class,
                () -> mDevicePolicyManager.setOrganizationId(""));
    }

    public void testThrowsWhenTryingToReSetOrganizationId() {
        mUiAutomation.adoptShellPermissionIdentity(PERMISSIONS_TO_ADOPT);

        mDevicePolicyManager.setOrganizationId("abc");
        final String firstEsid = mDevicePolicyManager.getEnrollmentSpecificId();
        assertThat(firstEsid).isNotEmpty();

        assertThrows(IllegalStateException.class,
                () -> mDevicePolicyManager.setOrganizationId("xyz"));
    }

    /**
     * This test tests that the platform calculates the ESID according to the specification and
     * does not, for example, return the same ESID regardless of the managing package.
     */
    public void testCorrectCalculationOfEsid() {
        mUiAutomation.adoptShellPermissionIdentity(PERMISSIONS_TO_ADOPT);
        mDevicePolicyManager.setOrganizationId(ORGANIZATION_ID);
        final String esidFromDpm = mDevicePolicyManager.getEnrollmentSpecificId();
        final String calculatedEsid = calculateEsid(ADMIN_RECEIVER_COMPONENT.getPackageName(),
                ORGANIZATION_ID);
        assertThat(esidFromDpm).isEqualTo(calculatedEsid);
    }

    private String calculateEsid(String profileOwnerPackage, String enterpriseIdString) {
        TelephonyManager telephonyService = mContext.getSystemService(TelephonyManager.class);
        assertThat(telephonyService).isNotNull();

        WifiManager wifiManager = mContext.getSystemService(WifiManager.class);
        assertThat(wifiManager).isNotNull();

        final byte[] serialNumber = getPaddedHardwareIdentifier(Build.getSerial()).getBytes();
        final byte[] imei = getPaddedHardwareIdentifier(telephonyService.getImei(0)).getBytes();
        final byte[] meid = getPaddedHardwareIdentifier(telephonyService.getMeid(0)).getBytes();

        final byte[] macAddress;
        final String[] macAddresses = wifiManager.getFactoryMacAddresses();
        if (macAddresses == null || macAddresses.length == 0) {
            macAddress = "".getBytes();
        } else {
            macAddress = macAddresses[0].getBytes();
        }

        final int totalIdentifiersLength = serialNumber.length + imei.length + meid.length
                + macAddress.length;
        final ByteBuffer fixedIdentifiers = ByteBuffer.allocate(totalIdentifiersLength);
        fixedIdentifiers.put(serialNumber);
        fixedIdentifiers.put(imei);
        fixedIdentifiers.put(meid);
        fixedIdentifiers.put(macAddress);

        final byte[] dpcPackage = getPaddedProfileOwnerName(profileOwnerPackage).getBytes();
        final byte[] enterpriseId = getPaddedEnterpriseId(enterpriseIdString).getBytes();
        final ByteBuffer info = ByteBuffer.allocate(dpcPackage.length + enterpriseId.length);
        info.put(dpcPackage);
        info.put(enterpriseId);
        final byte[] esidBytes = computeHkdf("HMACSHA256", fixedIdentifiers.array(), null,
                info.array(), 16);
        ByteBuffer esidByteBuffer = ByteBuffer.wrap(esidBytes);

        return encodeBase32(esidByteBuffer.getLong()) + encodeBase32(esidByteBuffer.getLong());
    }

    private static String getPaddedHardwareIdentifier(String hardwareIdentifier) {
        if (hardwareIdentifier == null) {
            hardwareIdentifier = "";
        }
        return String.format("%16s", hardwareIdentifier);
    }

    private static String getPaddedProfileOwnerName(String profileOwnerPackage) {
        return String.format("%64s", profileOwnerPackage);
    }

    private static String getPaddedEnterpriseId(String enterpriseId) {
        return String.format("%64s", enterpriseId);
    }

    // Copied from android.security.identity.Util, here to make sure Enterprise-Specific ID is
    // calculated according to spec.
    @NonNull
    private static byte[] computeHkdf(
            @NonNull String macAlgorithm, @NonNull final byte[] ikm, @NonNull final byte[] salt,
            @NonNull final byte[] info, int size) {
        Mac mac = null;
        try {
            mac = Mac.getInstance(macAlgorithm);
        } catch (NoSuchAlgorithmException e) {
            throw new RuntimeException("No such algorithm: " + macAlgorithm, e);
        }
        if (size > 255 * mac.getMacLength()) {
            throw new RuntimeException("size too large");
        }
        try {
            if (salt == null || salt.length == 0) {
                // According to RFC 5869, Section 2.2 the salt is optional. If no salt is provided
                // then HKDF uses a salt that is an array of zeros of the same length as the hash
                // digest.
                mac.init(new SecretKeySpec(new byte[mac.getMacLength()], macAlgorithm));
            } else {
                mac.init(new SecretKeySpec(salt, macAlgorithm));
            }
            byte[] prk = mac.doFinal(ikm);
            byte[] result = new byte[size];
            int ctr = 1;
            int pos = 0;
            mac.init(new SecretKeySpec(prk, macAlgorithm));
            byte[] digest = new byte[0];
            while (true) {
                mac.update(digest);
                mac.update(info);
                mac.update((byte) ctr);
                digest = mac.doFinal();
                if (pos + digest.length < size) {
                    System.arraycopy(digest, 0, result, pos, digest.length);
                    pos += digest.length;
                    ctr++;
                } else {
                    System.arraycopy(digest, 0, result, pos, size - pos);
                    break;
                }
            }
            return result;
        } catch (InvalidKeyException e) {
            throw new RuntimeException("Error MACing", e);
        }
    }

    private static final char[] ENCODE = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
            'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
            'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
            'Y', 'Z', '2', '3', '4', '5', '6', '7',
    };

    private static final char SEPARATOR = '-';
    private static final int LONG_SIZE = 13;
    private static final int GROUP_SIZE = 4;

    private static String encodeBase32(long input) {
        final char[] alphabet = ENCODE;

        /*
         * Make a character array with room for the separators between each
         * group.
         */
        final char[] encoded = new char[LONG_SIZE + (LONG_SIZE / GROUP_SIZE)];

        int index = encoded.length;
        for (int i = 0; i < LONG_SIZE; i++) {
            /*
             * Make sure we don't put a separator at the beginning. Since we're
             * building from the rear of the array, we use (LONG_SIZE %
             * GROUP_SIZE) to make the odd-size group appear at the end instead
             * of the beginning.
             */
            if (i > 0 && (i % GROUP_SIZE) == (LONG_SIZE % GROUP_SIZE)) {
                encoded[--index] = SEPARATOR;
            }

            /*
             * Extract 5 bits of data, then shift it out.
             */
            final int group = (int) (input & 0x1F);
            input >>>= 5;

            encoded[--index] = alphabet[group];
        }

        return String.valueOf(encoded);
    }
}
