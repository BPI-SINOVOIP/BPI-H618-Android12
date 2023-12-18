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


package com.android.car.bluetooth;

import static com.google.common.truth.Truth.assertThat;

import static org.mockito.Mockito.after;
import static org.mockito.Mockito.any;
import static org.mockito.Mockito.anyInt;
import static org.mockito.Mockito.anyString;
import static org.mockito.Mockito.never;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothManager;
import android.bluetooth.le.AdvertiseData;
import android.bluetooth.le.AdvertisingSetParameters;
import android.bluetooth.le.BluetoothLeAdvertiser;
import android.content.Context;
import android.content.Intent;
import android.content.SharedPreferences;
import android.content.res.Resources;
import android.os.ParcelUuid;

import com.android.dx.mockito.inline.extended.ExtendedMockito;

import org.junit.After;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.ArgumentCaptor;
import org.mockito.Captor;
import org.mockito.Mock;
import org.mockito.MockitoAnnotations;
import org.mockito.MockitoSession;
import org.mockito.junit.MockitoJUnitRunner;
import org.mockito.quality.Strictness;

import java.math.BigInteger;
import java.util.ArrayList;
import java.util.Base64;
import java.util.List;
import java.util.Map;

/**
 * Unit tests for {@link BluetoothFastPair}
 *
 * Run: atest BluetoothFastPairTest
 */
@RunWith(MockitoJUnitRunner.class)
public class BluetoothFastPairTest {

    static final byte[] TEST_ACCOUNT_KEY_1 = new byte[]{0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
            (byte) 0x88, (byte) 0x99, 0x00, (byte) 0xAA, (byte) 0xBB, (byte) 0xCC, (byte) 0xDD,
            (byte) 0xEE, (byte) 0xFF};
    static final byte[] TEST_ACCOUNT_KEY_2 = new byte[]{0x11, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44,
            0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, (byte) 0x88, (byte) 0x88};
    static final byte[] TEST_ACCOUNT_KEY_3 = new byte[]{0x04, 0x11, 0x22, 0x22, 0x33, 0x33, 0x44,
            0x44, 0x55, 0x55, 0x66, 0x66, 0x77, 0x77, (byte) 0x88, (byte) 0x88};


    static final byte[] TEST_KEY_1_EXPECTED_RESULT = new byte[]{0x0A, 0x42, (byte) 0x88, 0x10};
    static final byte[] TEST_KEY_12_EXPECTED_RESULT = new byte[]{0x2F, (byte) 0xBA, 0x06, 0x42,
            0x00};
    static final byte TEST_SALT = (byte) 0xC7;
    static final byte[] TEST_INPUT = {(byte) 0xF3, 0x0F, 0x4E, 0x78, 0x6C, 0x59, (byte) 0xA7,
            (byte) 0xBB, (byte) 0xF3, (byte) 0x87, 0x3B, 0x5A, 0x49, (byte) 0xBA, (byte) 0x97,
            (byte) 0xEA};
    static final byte[] TEST_OUTPUT = {(byte) 0xAC, (byte) 0x9A, 0x16, (byte) 0xF0, (byte) 0x95,
            0x3A, 0x3F, 0x22, 0x3D, (byte) 0xD1, 0x0C, (byte) 0xF5, 0x36, (byte) 0xE0, (byte) 0x9E,
            (byte) 0x9C};
    static final byte[] TEST_SHARED_SECRET = {(byte) 0xA0, (byte) 0xBA, (byte) 0xF0, (byte) 0xBB,
            (byte) 0x95, 0x1F, (byte) 0xF7, (byte) 0xB6, (byte) 0xCF, 0x5E, 0x3F, 0x45, 0x61,
            (byte) 0xC3, 0x32, 0x1D};
    static final byte[] TEST_PRIVATE_KEY_B = {0x02, (byte) 0xB4, 0x37, (byte) 0xB0, (byte) 0xED,
            (byte) 0xD6, (byte) 0xBB, (byte) 0xD4, 0x29, 0x06, 0x4A, 0x4E, 0x52, (byte) 0x9F,
            (byte) 0xCB, (byte) 0xF1, (byte) 0xC4, (byte) 0x8D, 0x0D, 0x62, 0x49, 0x24, (byte) 0xD5,
            (byte) 0x92, 0x27, 0x4B, 0x7E, (byte) 0xD8, 0x11, (byte) 0x93, (byte) 0xD7, 0x63
    };
    static final byte[] TEST_PUBLIC_KEY_A = {0x36, (byte) 0xAC, 0x68, 0x2C, 0x50, (byte) 0x82, 0x15,
            0x66, (byte) 0x8F, (byte) 0xBE, (byte) 0xFE, 0x24, 0x7D, 0x01, (byte) 0xD5, (byte) 0xEB,
            (byte) 0x96, (byte) 0xE6, 0x31, (byte) 0x8E, (byte) 0x85, 0x5B, 0x2D, 0x64, (byte) 0xB5,
            0x19, 0x5D, 0x38, (byte) 0xEE, 0x7E, 0x37, (byte) 0xBE, 0x18, 0x38, (byte) 0xC0,
            (byte) 0xB9, 0x48, (byte) 0xC3, (byte) 0xF7, 0x55, 0x20, (byte) 0xE0, 0x7E, 0x70,
            (byte) 0xF0, 0x72, (byte) 0x91, 0x41, (byte) 0x9A, (byte) 0xCE, 0x2D, 0x28, 0x14, 0x3C,
            0x5A, (byte) 0xDB, 0x2D, (byte) 0xBD, (byte) 0x98, (byte) 0xEE, 0x3C, (byte) 0x8E, 0x4F,
            (byte) 0xBF};
    static final byte[] TEST_GENERATED_KEY = {(byte) 0xB0, 0x7F, 0x1F, 0x17, (byte) 0xC2, 0x36,
            (byte) 0xCB, (byte) 0xD3, 0x35, 0x23, (byte) 0xC5, 0x15, (byte) 0xF3, 0x50, (byte) 0xAE,
            0x57};
    static final String TEST_PUBLIC_KEY_A_BASE64 = Base64.getEncoder()
            .encodeToString(TEST_PUBLIC_KEY_A);
    static final String TEST_PRIVATE_KEY_B_BASE64 = Base64.getEncoder()
            .encodeToString(TEST_PRIVATE_KEY_B);
    static final String TEST_LOCAL_ADDRESS_STRING = "00:11:22:33:44:55";
    static final byte[] TEST_LOCAL_ADDRESS = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};
    static final byte[] TEST_REMOTE_ADDRESS = {0x66, 0x77, (byte) 0x88, (byte) 0x99, (byte) 0xAA,
            (byte) 0xBB};
    static final byte[] TEST_PAIRING_REQUEST = {0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x66, 0x77, (byte) 0x88, (byte) 0x99, (byte) 0xAA, (byte) 0xBB, 0x00, 0x00};
    static final byte[] TEST_PAIRING_KEY = {0x02, 0x01, 0x02, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    static final int TEST_PIN_NUMBER = 66051;
    static final int TEST_MODEL_ID = 4386;
    static final byte[] TEST_MODEL_ID_BYTES = {0x00, 0x11, 0x22};
    byte[] mAdvertisementExpectedResults = new byte[]{0x00, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x11, 0x00};
    @Mock
    Context mMockContext;

    @Mock
    Resources mMockResources;

    @Mock
    SharedPreferences mMockSharedPreferences;

    @Mock
    SharedPreferences.Editor mMockSharedPreferencesEditor;

    @Mock
    BluetoothManager mMockBluetoothManager;

    @Mock
    FastPairGattServer.Callbacks mMockGattCallbacks;

    @Mock
    BluetoothAdapter mMockBluetoothAdapter;

    @Mock
    BluetoothDevice mMockBluetoothDevice;

    @Mock
    BluetoothLeAdvertiser mMockLeAdvertiser;

    @Captor ArgumentCaptor<AdvertisingSetParameters> mAdvertisingSetParameters;
    @Captor ArgumentCaptor<AdvertiseData> mAdvertiseData;

    FastPairAdvertiser mTestFastPairAdvertiser;
    FastPairProvider mTestFastPairProvider;
    FastPairGattServer mTestGattServer;

    MockitoSession mMockitoSession;

    @Before
    public void setUp() {
        MockitoAnnotations.initMocks(this);
        when(mMockContext.getSharedPreferences(anyString(), anyInt()))
                .thenReturn(mMockSharedPreferences);
        when(mMockSharedPreferences.getInt(any(), anyInt())).thenReturn(2);
        when(mMockSharedPreferences.getString(any(), any()))
                .thenReturn(new BigInteger(TEST_ACCOUNT_KEY_1).toString())
                .thenReturn(new BigInteger(TEST_ACCOUNT_KEY_2).toString());
        when(mMockSharedPreferences.edit()).thenReturn(mMockSharedPreferencesEditor);

        when(mMockContext.getResources()).thenReturn(mMockResources);
        when(mMockContext.getSystemService(BluetoothManager.class))
                .thenReturn(mMockBluetoothManager);
        when(mMockResources.getInteger(anyInt())).thenReturn(0x001122);
        when(mMockResources.getBoolean(anyInt())).thenReturn(true);
        when(mMockBluetoothManager.getAdapter()).thenReturn(mMockBluetoothAdapter);
        when(mMockBluetoothAdapter.getBluetoothLeAdvertiser()).thenReturn(mMockLeAdvertiser);
        when(mMockBluetoothAdapter.getName()).thenReturn("name");
        when(mMockBluetoothAdapter.getAddress()).thenReturn("00:11:22:33:FF:EE");
        when(mMockBluetoothAdapter.getRemoteDevice(any(String.class))).thenReturn(
                mMockBluetoothDevice);
        when(mMockBluetoothAdapter.getRemoteDevice(any(byte[].class))).thenReturn(
                mMockBluetoothDevice);

        mMockitoSession = ExtendedMockito.mockitoSession()
                .strictness(Strictness.WARN)
                .spyStatic(BluetoothAdapter.class)
                .startMocking();
        ExtendedMockito.doReturn(mMockBluetoothAdapter).when(() ->
                BluetoothAdapter.getDefaultAdapter());

        mTestFastPairAdvertiser = new FastPairAdvertiser(mMockContext, TEST_MODEL_ID, null);
        mTestFastPairProvider = new FastPairProvider(mMockContext);
        mTestGattServer = new FastPairGattServer(mMockContext, TEST_MODEL_ID,
                TEST_PRIVATE_KEY_B_BASE64, mMockGattCallbacks, true);
    }

    @After
    public void tearDown() {
        mMockitoSession.finishMocking();
    }

    @Test
    public void bloomFilterOneKeyTest() {
        List<FastPairUtils.AccountKey> testKeys = new ArrayList();
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1));

        byte[] bloomResults = FastPairUtils.bloom(testKeys, TEST_SALT);
        assertThat(bloomResults).isEqualTo(TEST_KEY_1_EXPECTED_RESULT);
    }

    @Test
    public void bloomFilterTwoKeyTest() {
        List<FastPairUtils.AccountKey> testKeys = new ArrayList();
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1));
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_2));

        byte[] bloomResults = FastPairUtils.bloom(testKeys, TEST_SALT);
        assertThat(bloomResults).isEqualTo(TEST_KEY_12_EXPECTED_RESULT);
    }

    @Test
    public void readAccountKeysTest() {
        List<FastPairUtils.AccountKey> testKeys = FastPairUtils.readStoredAccountKeys(mMockContext);
        assertThat(testKeys.size()).isEqualTo(2);
        assertThat(testKeys.get(0).key).isEqualTo(TEST_ACCOUNT_KEY_1);
        assertThat(testKeys.get(1).key).isEqualTo(TEST_ACCOUNT_KEY_2);
    }

    @Test
    public void getAccountKeyTest() {
        byte[] advertisementResults = FastPairUtils.getAccountKeyAdvertisement(mMockContext);
        byte salt = advertisementResults[advertisementResults.length - 1];
        List<FastPairUtils.AccountKey> testKeys = new ArrayList();
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1));
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_2));

        byte[] bloomResults = FastPairUtils.bloom(testKeys, salt);
        System.arraycopy(bloomResults, 0, mAdvertisementExpectedResults, 2,
                bloomResults.length);
        mAdvertisementExpectedResults[mAdvertisementExpectedResults.length - 1] = salt;

        assertThat(advertisementResults).isEqualTo(mAdvertisementExpectedResults);
    }

    @Test
    public void testGetBytesFromAddress() {
        byte[] conversionResults = FastPairUtils.getBytesFromAddress(TEST_LOCAL_ADDRESS_STRING);
        assertThat(conversionResults).isEqualTo(TEST_LOCAL_ADDRESS);
    }

    @Test
    public void testAccountKeyCreation() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1);
        assertThat(testKey.toBytes()).isEqualTo(TEST_ACCOUNT_KEY_1);
    }

    @Test
    public void testSymmetricEncryption() {
        mTestGattServer.setSharedSecretKey(TEST_SHARED_SECRET);
        byte[] testEncoded = mTestGattServer.encrypt(TEST_INPUT);
        assertThat(testEncoded).isEqualTo(TEST_OUTPUT);

        byte[] testDecoded = mTestGattServer.decrypt(testEncoded);
        assertThat(testDecoded).isEqualTo(TEST_INPUT);

    }

    @Test
    public void tessECDHKeyGeneration() {
        FastPairUtils.AccountKey generatedKey = mTestGattServer
                .calculateAntiSpoofing(TEST_PRIVATE_KEY_B, TEST_PUBLIC_KEY_A);
        assertThat(generatedKey.toBytes()).isEqualTo(TEST_GENERATED_KEY);

    }

    @Test
    public void testStoredKeySelection() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);
        mTestGattServer.setSharedSecretKey(TEST_SHARED_SECRET);
        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                (new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_2)).getKeySpec())).isFalse();

        assertThat(mTestGattServer.processKeyBasedPairing(encryptedRequest)).isTrue();
    }


    @Test
    public void testNoValidKey() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_SHARED_SECRET);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);
        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                (new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1)).getKeySpec())).isFalse();
        assertThat(mTestGattServer.processKeyBasedPairing(encryptedRequest)).isFalse();
    }

    @Test
    public void testDisableAfter10Failures() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_2);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);
        assertThat(mTestGattServer.processKeyBasedPairing(encryptedRequest)).isTrue();

        for (int i = 0; i < 10; i++) {
            assertThat(mTestGattServer.processKeyBasedPairing(TEST_PAIRING_REQUEST)).isFalse();
        }

        assertThat(mTestGattServer.processKeyBasedPairing(encryptedRequest)).isFalse();
    }

    @Test
    public void testIgnoreAfterTimeout() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_SHARED_SECRET);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);

        byte[] encryptedPairingKey = mTestGattServer.encrypt(TEST_PAIRING_KEY);

        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                testKey.getKeySpec())).isTrue();
        //sendTimeout
        verify(mMockBluetoothDevice, after(11000).times(0)).setPairingConfirmation(false);

        assertThat(mTestGattServer.processPairingKey(encryptedPairingKey)).isFalse();
    }

    @Test
    public void testInvalidPairingKey() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_SHARED_SECRET);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);

        byte[] encryptedPairingKey = mTestGattServer.encrypt(TEST_PAIRING_KEY);

        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                testKey.getKeySpec())).isTrue();
        //send Wrong Pairing Key
        sendPairingKey(-1);
        mTestGattServer.processPairingKey(encryptedPairingKey);
        verify(mMockBluetoothDevice).setPairingConfirmation(false);
    }

    @Test
    public void testValidPairingKeyAutoAccept() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_SHARED_SECRET);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);

        byte[] encryptedPairingKey = mTestGattServer.encrypt(TEST_PAIRING_KEY);

        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                testKey.getKeySpec())).isTrue();

        sendPairingKey(TEST_PIN_NUMBER);
        mTestGattServer.processPairingKey(encryptedPairingKey);

        verify(mMockBluetoothDevice).setPairingConfirmation(true);
    }

    @Test
    public void testValidPairingKeyNoAutoAccept() {
        mTestGattServer = new FastPairGattServer(mMockContext, TEST_MODEL_ID,
                TEST_PRIVATE_KEY_B_BASE64, mMockGattCallbacks, false);

        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_SHARED_SECRET);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);

        byte[] encryptedPairingKey = mTestGattServer.encrypt(TEST_PAIRING_KEY);

        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                testKey.getKeySpec())).isTrue();

        sendPairingKey(TEST_PIN_NUMBER);
        mTestGattServer.processPairingKey(encryptedPairingKey);

        verify(mMockBluetoothDevice, never()).setPairingConfirmation(true);

    }

    @Test
    public void receivedAccountKey() {
        FastPairUtils.AccountKey testKey = new FastPairUtils.AccountKey(TEST_SHARED_SECRET);
        mTestGattServer.setSharedSecretKey(testKey.toBytes());
        byte[] encryptedRequest = mTestGattServer.encrypt(TEST_PAIRING_REQUEST);

        byte[] encryptedPairingKey = mTestGattServer.encrypt(TEST_PAIRING_KEY);

        byte[] encryptedAccountKey = mTestGattServer.encrypt(TEST_ACCOUNT_KEY_3);

        assertThat(mTestGattServer.validatePairingRequest(encryptedRequest,
                testKey.getKeySpec())).isTrue();

        sendPairingKey(TEST_PIN_NUMBER);
        mTestGattServer.processPairingKey(encryptedPairingKey);

        verify(mMockBluetoothDevice).setPairingConfirmation(true);
        mTestGattServer.processAccountKey(encryptedAccountKey);
        verify(mMockSharedPreferences).edit();

    }

    @Test
    public void testAdvertiseAccountKeys() {
        mTestFastPairAdvertiser.advertiseAccountKeys();
        verify(mMockLeAdvertiser).startAdvertisingSet(mAdvertisingSetParameters.capture(),
                mAdvertiseData.capture(), any(), any(), any(), any());
        assertThat(mAdvertisingSetParameters.getValue().getInterval())
                .isEqualTo(AdvertisingSetParameters.INTERVAL_MEDIUM);
        Map<ParcelUuid, byte[]> serviceData = mAdvertiseData.getValue().getServiceData();
        assertThat(serviceData.size()).isEqualTo(1);
        byte[] advertisementResults = serviceData.get(FastPairAdvertiser.FastPairServiceUuid);
        byte salt = advertisementResults[advertisementResults.length - 1];
        List<FastPairUtils.AccountKey> testKeys = new ArrayList();
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_1));
        testKeys.add(new FastPairUtils.AccountKey(TEST_ACCOUNT_KEY_2));

        byte[] bloomResults = FastPairUtils.bloom(testKeys, salt);
        System.arraycopy(bloomResults, 0, mAdvertisementExpectedResults, 2,
                bloomResults.length);
        mAdvertisementExpectedResults[mAdvertisementExpectedResults.length - 1] = salt;

        assertThat(advertisementResults).isEqualTo(mAdvertisementExpectedResults);
    }

    @Test
    public void testAdvertiseModelId() {
        mTestFastPairAdvertiser.advertiseModelId();
        verify(mMockLeAdvertiser).startAdvertisingSet(mAdvertisingSetParameters.capture(),
                mAdvertiseData.capture(), any(), any(), any(), any());
        assertThat(mAdvertisingSetParameters.getValue().getInterval())
                .isEqualTo(AdvertisingSetParameters.INTERVAL_LOW);
        Map<ParcelUuid, byte[]> serviceData = mAdvertiseData.getValue().getServiceData();
        assertThat(serviceData.size()).isEqualTo(1);
        assertThat(serviceData.get(FastPairAdvertiser.FastPairServiceUuid))
                .isEqualTo(TEST_MODEL_ID_BYTES);
    }

    void sendPairingKey(int pairingKey) {
        Intent pairingRequest = new Intent(BluetoothDevice.ACTION_PAIRING_REQUEST);
        pairingRequest.putExtra(BluetoothDevice.EXTRA_PAIRING_KEY, pairingKey);
        pairingRequest.putExtra(BluetoothDevice.EXTRA_DEVICE, mMockBluetoothDevice);
        mTestGattServer.mPairingAttemptsReceiver.onReceive(mMockContext, pairingRequest);
    }
}
