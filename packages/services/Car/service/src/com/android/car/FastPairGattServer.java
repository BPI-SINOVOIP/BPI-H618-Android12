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

import static com.android.car.bluetooth.FastPairUtils.AccountKey;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothGatt;
import android.bluetooth.BluetoothGattCharacteristic;
import android.bluetooth.BluetoothGattDescriptor;
import android.bluetooth.BluetoothGattServer;
import android.bluetooth.BluetoothGattServerCallback;
import android.bluetooth.BluetoothGattService;
import android.bluetooth.BluetoothManager;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.Handler;
import android.os.ParcelUuid;
import android.util.Base64;
import android.util.IndentingPrintWriter;
import android.util.Log;

import com.android.car.CarServiceUtils;

import java.math.BigInteger;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.security.KeyFactory;
import java.security.KeyPairGenerator;
import java.security.MessageDigest;
import java.security.PrivateKey;
import java.security.PublicKey;
import java.security.interfaces.ECPublicKey;
import java.security.spec.ECParameterSpec;
import java.security.spec.ECPoint;
import java.security.spec.ECPrivateKeySpec;
import java.security.spec.ECPublicKeySpec;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Random;

import javax.crypto.Cipher;
import javax.crypto.KeyAgreement;
import javax.crypto.spec.SecretKeySpec;

/**
 * The FastPairGattServer is responsible for all 2 way communications with the Fast Pair Seeker.
 * It is running in the background over BLE whenever the Fast Pair Service is running, waiting for a
 * Seeker to connect, after which time it manages the authentication an performs the steps as
 * required by the Fast Pair Specification.
 */
class FastPairGattServer {
    // Service ID assigned for FastPair.
    public static final ParcelUuid FAST_PAIR_SERVICE_UUID = ParcelUuid
            .fromString("0000FE2C-0000-1000-8000-00805f9b34fb");
    public static final ParcelUuid FAST_PAIR_MODEL_ID_UUID = ParcelUuid
            .fromString("FE2C1233-8366-4814-8EB0-01DE32100BEA");
    public static final ParcelUuid KEY_BASED_PAIRING_UUID = ParcelUuid
            .fromString("FE2C1234-8366-4814-8EB0-01DE32100BEA");
    public static final ParcelUuid PASSKEY_UUID = ParcelUuid
            .fromString("FE2C1235-8366-4814-8EB0-01DE32100BEA");
    public static final ParcelUuid ACCOUNT_KEY_UUID = ParcelUuid
            .fromString("FE2C1236-8366-4814-8EB0-01DE32100BEA");
    public static final ParcelUuid CLIENT_CHARACTERISTIC_CONFIG = ParcelUuid
            .fromString("00002902-0000-1000-8000-00805f9b34fb");
    public static final ParcelUuid DEVICE_NAME_CHARACTERISTIC_CONFIG = ParcelUuid
            .fromString("00002A00-0000-1000-8000-00805f9b34fb");
    private static final String TAG = "FastPairGattServer";
    private static final boolean DBG = FastPairUtils.DBG;
    private static final int MAX_KEY_COUNT = 10;
    private static final int KEY_LIFESPAN = 10_000;

    private final boolean mAutomaticPasskeyConfirmation;
    private final byte[] mModelId;
    private final String mPrivateAntiSpoof;
    private final Context mContext;

    private ArrayList<AccountKey> mKeys = new ArrayList<>();
    private BluetoothGattServer mBluetoothGattServer;
    private BluetoothManager mBluetoothManager;
    private int mPairingPasskey = -1;
    private int mFailureCount = 0;
    private int mSuccessCount = 0;
    private BluetoothGattService mFastPairService = new BluetoothGattService(
            FAST_PAIR_SERVICE_UUID.getUuid(), BluetoothGattService.SERVICE_TYPE_PRIMARY);
    private Callbacks mCallbacks;
    private SecretKeySpec mSharedSecretKey;
    private byte[] mEncryptedResponse;
    private BluetoothDevice mLocalRpaDevice;
    private BluetoothDevice mRemotePairingDevice;
    private BluetoothDevice mRemoteGattDevice;

    interface Callbacks {
        /**
         * Notify the Provider of completion to a GATT session
         * @param successful
         */
        void onPairingCompleted(boolean successful);
    }

    /**
     * Check if a client is connected to this GATT server
     * @return true if connected;
     */
    public boolean isConnected() {
        return (mRemoteGattDevice != null);
    }

    public void updateLocalRpa(BluetoothDevice device) {
        mLocalRpaDevice = device;
    }

    private Runnable mClearSharedSecretKey = new Runnable() {
        @Override
        public void run() {
            mSharedSecretKey = null;
        }
    };
    private final Handler mHandler = new Handler(
            CarServiceUtils.getHandlerThread(FastPairUtils.THREAD_NAME).getLooper());
    private BluetoothGattCharacteristic mModelIdCharacteristic;
    private BluetoothGattCharacteristic mKeyBasedPairingCharacteristic;
    private BluetoothGattCharacteristic mPasskeyCharacteristic;
    private BluetoothGattCharacteristic mAccountKeyCharacteristic;
    private BluetoothGattCharacteristic mDeviceNameCharacteristic;

    /**
     * GATT server callbacks responsible for servicing read and write calls from the remote device
     */
    private BluetoothGattServerCallback mBluetoothGattServerCallback =
            new BluetoothGattServerCallback() {
        @Override
        public void onConnectionStateChange(BluetoothDevice device, int status, int newState) {
            super.onConnectionStateChange(device, status, newState);
            Log.d(TAG, "onConnectionStateChange " + newState + "Device: " + device.toString());
            if (newState == 0) {
                mPairingPasskey = -1;
                mSharedSecretKey = null;
                mRemoteGattDevice = null;
                mCallbacks.onPairingCompleted(false);
            } else if (newState > 0) {
                mRemoteGattDevice = device;
            }
        }

        @Override
        public void onCharacteristicReadRequest(BluetoothDevice device, int requestId, int offset,
                BluetoothGattCharacteristic characteristic) {
            super.onCharacteristicReadRequest(device, requestId, offset, characteristic);
            if (DBG) {
                Log.d(TAG, "onCharacteristicReadRequest");
            }
            if (characteristic == mModelIdCharacteristic) {
                if (DBG) {
                    Log.d(TAG, "reading model ID");
                }
            }
            mBluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                    characteristic.getValue());
        }


        @Override
        public void onCharacteristicWriteRequest(BluetoothDevice device, int requestId,
                BluetoothGattCharacteristic characteristic, boolean preparedWrite,
                boolean responseNeeded,
                int offset, byte[] value) {
            super.onCharacteristicWriteRequest(device, requestId, characteristic, preparedWrite,
                    responseNeeded, offset, value);
            if (DBG) {
                Log.d(TAG, "onWrite uuid()" + characteristic.getUuid() + "Length" + value.length);
            }
            if (characteristic == mAccountKeyCharacteristic) {
                if (DBG) {
                    Log.d(TAG, "onWriteAccountKeyCharacteristic");
                }
                processAccountKey(value);

                mBluetoothGattServer
                        .sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                                characteristic.getValue());

            } else if (characteristic == mKeyBasedPairingCharacteristic) {
                if (DBG) {
                    Log.d(TAG, "KeyBasedPairingCharacteristic");
                }
                processKeyBasedPairing(value);
                mKeyBasedPairingCharacteristic.setValue(mEncryptedResponse);
                mBluetoothGattServer
                        .sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                                mEncryptedResponse);
                mBluetoothGattServer
                        .notifyCharacteristicChanged(device, mDeviceNameCharacteristic, false);
                mBluetoothGattServer
                        .notifyCharacteristicChanged(device, mKeyBasedPairingCharacteristic, false);

            } else if (characteristic == mPasskeyCharacteristic) {
                if (DBG) {
                    Log.d(TAG, "onWritePasskey" + characteristic.getUuid());
                }
                mBluetoothGattServer
                        .sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                                mEncryptedResponse);
                processPairingKey(value);
                mBluetoothGattServer
                        .notifyCharacteristicChanged(device, mPasskeyCharacteristic, false);

            } else {
                Log.w(TAG, "onWriteOther" + characteristic.getUuid());
            }
        }

        @Override
        public void onDescriptorWriteRequest(BluetoothDevice device, int requestId,
                BluetoothGattDescriptor descriptor, boolean preparedWrite, boolean responseNeeded,
                int offset, byte[] value) {
            if (DBG) {
                Log.d(TAG, "onDescriptorWriteRequest");
            }
            mBluetoothGattServer.sendResponse(device, requestId, BluetoothGatt.GATT_SUCCESS, offset,
                    descriptor.getValue());
        }
    };

    /**
     *  Receive incoming pairing requests such that we can confirm Keys match.
     */
    BroadcastReceiver mPairingAttemptsReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            if (DBG) {
                Log.d(TAG, intent.getAction());
            }
            if (BluetoothDevice.ACTION_PAIRING_REQUEST.equals(intent.getAction())) {
                if (DBG) {
                    Log.d(TAG, "PairingCode " + intent
                                    .getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY, -1));
                }
                mRemotePairingDevice = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
                mPairingPasskey = intent.getIntExtra(BluetoothDevice.EXTRA_PAIRING_KEY, -1);
            }
        }
    };

    /**
     * FastPairGattServer
     * @param context user specific context on which to make callse
     * @param modelId assigned Fast Pair Model ID
     * @param antiSpoof assigned Fast Pair private Anti Spoof key
     * @param callbacks callbacks used to report back current pairing status
     * @param automaticAcceptance automatically accept an incoming pairing request that has been
     *     authenticated through the Fast Pair protocol without further user interaction.
     */
    FastPairGattServer(Context context, int modelId, String antiSpoof,
            Callbacks callbacks, boolean automaticAcceptance) {
        mContext = context;
        mCallbacks = callbacks;
        mPrivateAntiSpoof = antiSpoof;
        mAutomaticPasskeyConfirmation = automaticAcceptance;
        mBluetoothManager = context.getSystemService(BluetoothManager.class);

        mBluetoothGattServer = mBluetoothManager
                .openGattServer(context, mBluetoothGattServerCallback);
        if (DBG) {
            Log.d(TAG, "mBTManager: " + mBluetoothManager.toString() + " GATT: "
                    + mBluetoothGattServer);
        }
        ByteBuffer modelIdBytes = ByteBuffer.allocate(4).order(ByteOrder.LITTLE_ENDIAN).putInt(
                modelId);
        mModelId = Arrays.copyOfRange(modelIdBytes.array(), 0, 3);
        setup();
    }

    void setSharedSecretKey(byte[] key) {
        mSharedSecretKey = new SecretKeySpec(key, "AES");
        mHandler.postDelayed(mClearSharedSecretKey, KEY_LIFESPAN);
    }

    /**
     * Utilize the key set via setSharedSecretKey to attempt to encrypt the provided data
     * @param decoded data to be encrypted
     * @return encrypted data upon success; null otherwise
     */
    byte[] encrypt(byte[] decoded) {
        try {
            Cipher cipher = Cipher.getInstance("AES/ECB/NoPadding");
            cipher.init(Cipher.ENCRYPT_MODE, mSharedSecretKey);
            mHandler.removeCallbacks(mClearSharedSecretKey);
            mHandler.postDelayed(mClearSharedSecretKey, KEY_LIFESPAN);
            return cipher.doFinal(decoded);

        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }
        mHandler.removeCallbacks(mClearSharedSecretKey);
        mSharedSecretKey = null;
        return null;
    }
    /**
     * Utilize the key set via setSharedSecretKey to attempt to decrypt the provided data
     * @param encoded data to be decrypted
     * @return decrypted data upon success; null otherwise
     */
    byte[] decrypt(byte[] encoded) {
        try {
            Cipher cipher = Cipher.getInstance("AES/ECB/NoPadding");
            cipher.init(Cipher.DECRYPT_MODE, mSharedSecretKey);
            mHandler.removeCallbacks(mClearSharedSecretKey);
            mHandler.postDelayed(mClearSharedSecretKey, KEY_LIFESPAN);
            return cipher.doFinal(encoded);

        } catch (Exception e) {
            Log.e(TAG, e.toString());
        }
        mHandler.removeCallbacks(mClearSharedSecretKey);
        mSharedSecretKey = null;
        return null;
    }

    /**
     * The final step of the Fast Pair procedure involves receiving an account key from the
     * Fast Pair seeker, authenticating it, and then storing it for future use.
     * @param accountKey
     */
    void processAccountKey(byte[] accountKey) {
        byte[] decodedAccountKey = decrypt(accountKey);
        if (decodedAccountKey != null && decodedAccountKey[0] == 0x04) {
            if (DBG) {
                Log.d(TAG, "ReceivedAccountKey" + decodedAccountKey[0]);
            }
            FastPairUtils.AccountKey receivedKey = new FastPairUtils.AccountKey(decodedAccountKey);
            if (!mKeys.contains(receivedKey)) {
                mKeys.add(receivedKey);
            }
            // due to space restrictions in the protocol we can only store 10 keys
            while (mKeys.size() > MAX_KEY_COUNT) {
                mKeys.remove(0);
            }
            FastPairUtils.writeStoredAccountKeys(mContext, mKeys);
            mSuccessCount++;
        } else {
            if (DBG) {
                Log.d(TAG, "Invalid Account Key");
            }
        }
    }

    /**
     * New pairings based upon model ID requires the Fast Pair provider to authenticate to the
     * seeker that it is in possession of the private key associated with the model ID advertised,
     * this is accomplished via Eliptic-curve Diffie-Hellman
     * @param localPrivateKey
     * @param remotePublicKey
     * @return
     */
    AccountKey calculateAntiSpoofing(byte[] localPrivateKey, byte[] remotePublicKey) {
        try {
            if (DBG) {
                Log.d(TAG, "Calculating secret key from remotePublicKey");
            }
            // Initialize the EC key generator
            KeyFactory keyFactory = KeyFactory.getInstance("EC");
            KeyPairGenerator kpg = KeyPairGenerator.getInstance("EC");
            ECParameterSpec ecParameterSpec = ((ECPublicKey) kpg.generateKeyPair().getPublic())
                    .getParams();
            // Use the private anti-spoofing key
            ECPrivateKeySpec ecPrivateKeySpec = new ECPrivateKeySpec(
                    new BigInteger(1, localPrivateKey),
                    ecParameterSpec);
            // Calculate the public point utilizing the data received from the remote device
            ECPoint publicPoint = new ECPoint(new BigInteger(1, Arrays.copyOf(remotePublicKey, 32)),
                    new BigInteger(1, Arrays.copyOfRange(remotePublicKey, 32, 64)));
            ECPublicKeySpec ecPublicKeySpec = new ECPublicKeySpec(publicPoint, ecParameterSpec);
            PrivateKey privateKey = keyFactory.generatePrivate(ecPrivateKeySpec);
            PublicKey publicKey = keyFactory.generatePublic(ecPublicKeySpec);

            // Generate a shared secret
            KeyAgreement keyAgreement = KeyAgreement.getInstance("ECDH");
            keyAgreement.init(privateKey);
            keyAgreement.doPhase(publicKey, true);
            byte[] sharedSecret = keyAgreement.generateSecret();

            // Use the first 16 bytes of a hash of the shared secret as the session key
            final byte[] digest = MessageDigest.getInstance("SHA-256").digest(sharedSecret);

            byte[] AESAntiSpoofingKey = Arrays.copyOf(digest, 16);
            if (DBG) {
                Log.d(TAG, "Key calculated");
            }
            return new AccountKey(AESAntiSpoofingKey);
        } catch (Exception e) {
            Log.w(TAG, e.toString());
            return null;
        }
    }

    /**
     * Determine if this pairing request is based on the anti-spoof keys associated with the model
     * id or stored account keys.
     * @param accountKey
     * @return
     */
    boolean processKeyBasedPairing(byte[] pairingRequest) {
        if (mFailureCount >= 10) return false;

        List<SecretKeySpec> possibleKeys = new ArrayList<>();
        if (pairingRequest.length == 80) {
            // if the pairingRequest is 80 bytes long try the anit-spoof key
            final byte[] remotePublicKey = Arrays.copyOfRange(pairingRequest, 16, 80);

            possibleKeys
                    .add(calculateAntiSpoofing(Base64.decode(mPrivateAntiSpoof, 0), remotePublicKey)
                            .getKeySpec());
        } else {
            // otherwise the pairing request is the encrypted request, try all the stored account
            // keys
            List<AccountKey> storedAccountKeys = FastPairUtils.readStoredAccountKeys(mContext);
            for (AccountKey key : storedAccountKeys) {
                possibleKeys.add(new SecretKeySpec(key.toBytes(), "AES"));
            }
        }

        byte[] encryptedRequest = Arrays.copyOfRange(pairingRequest, 0, 16);
        if (DBG) {
            Log.d(TAG, "Checking " + possibleKeys.size() + "Keys");
        }
        // check all the keys for a valid pairing request
        for (SecretKeySpec key : possibleKeys) {
            if (DBG) {
                Log.d(TAG, "Checking possibleKey");
            }
            if (validatePairingRequest(encryptedRequest, key)) {
                return true;
            }
        }
        Log.w(TAG, "No Matching Key found");
        mFailureCount++;
        mSharedSecretKey = null;
        return false;
    }

    /**
     * Check if the pairing request is a valid request.
     * A request is valid if its decrypted value is of type 0x00 or 0x10 and it contains either the
     * seekers public or current BLE address
     * @param encryptedRequest the request to decrypt and validate
     * @param secretKeySpec the key to use while attempting to decrypt the request
     * @return true if the key matches, false otherwise
     */
    boolean validatePairingRequest(byte[] encryptedRequest, SecretKeySpec secretKeySpec) {
        // Decrypt the request
        mSharedSecretKey = secretKeySpec;
        byte[] decryptedRequest = decrypt(encryptedRequest);
        if (decryptedRequest == null) {
            return false;
        }
        if (DBG) {
            Log.d(TAG, "Decrypted" + decryptedRequest[0] + "Flags" + decryptedRequest[1]);
        }
        // Check that the request is either a Key-based Pairing Request or an Action Request
        if (decryptedRequest[0] == 0 || decryptedRequest[0] == 0x10) {
            String localAddress = BluetoothAdapter.getDefaultAdapter().getAddress();
            byte[] localAddressBytes = FastPairUtils.getBytesFromAddress(localAddress);
            // Extract the remote address bytes from the message
            byte[] remoteAddressBytes = Arrays.copyOfRange(decryptedRequest, 2, 8);
            BluetoothDevice localDevice = BluetoothAdapter.getDefaultAdapter()
                    .getRemoteDevice(localAddress);
            BluetoothDevice reportedDevice = BluetoothAdapter.getDefaultAdapter()
                    .getRemoteDevice(remoteAddressBytes);
            if (DBG) {
                Log.d(TAG, "Local RPA = " + mLocalRpaDevice);
                Log.d(TAG, "Decrypted, LocalMacAddress" + localAddress + "remoteAddress"
                        + reportedDevice.toString());
            }
            // Test that the received device address matches this devices address
            if (reportedDevice.equals(localDevice) || reportedDevice.equals(mLocalRpaDevice)) {
                if (DBG) {
                    Log.d(TAG, "SecretKey Validated");
                }

                // encrypt and respond to the seeker with the local public address
                byte[] rawResponse = new byte[16];
                new Random().nextBytes(rawResponse);
                rawResponse[0] = 0x01;
                System.arraycopy(localAddressBytes, 0, rawResponse, 1, 6);
                mEncryptedResponse = encrypt(rawResponse);
                return encryptedRequest != null;
            }
        }
        return false;
    }

    /**
     * Extract the 6 digit Bluetooth Simple Secure Passkey from the received message and confirm
     * it matches the key received through the Bluetooth pairing procedure.
     * If the passkeys match and automatic passkey confirmation is enabled, approve of the pairing.
     * If the passkeys do not match reject the pairing.
     * @param pairingKey
     * @return true if the procedure completed, although pairing may not have been approved
     */
    boolean processPairingKey(byte[] pairingKey) {
        byte[] decryptedRequest = decrypt(pairingKey);
        if (decryptedRequest == null) {
            return false;
        }
        int passkey = Byte.toUnsignedInt(decryptedRequest[1]) * 65536
                + Byte.toUnsignedInt(decryptedRequest[2]) * 256
                + Byte.toUnsignedInt(decryptedRequest[3]);

        if (DBG) {
            Log.d(TAG, "PairingKey , MessageType " + decryptedRequest[0]
                    + "FastPair Passkey = " + passkey + "Bluetooth Passkey = " + mPairingPasskey);
        }
        // compare the Bluetooth received passkey with the Fast Pair received passkey
        if (mPairingPasskey == passkey) {
            if (mAutomaticPasskeyConfirmation) {
                if (DBG) {
                    Log.d(TAG, "Passkeys match, accepting");
                }
                mRemotePairingDevice.setPairingConfirmation(true);
            }
        } else {
            Log.w(TAG, "Passkeys don't match, rejecting");
            mRemotePairingDevice.setPairingConfirmation(false);
        }

        // Send an encrypted response to the seeker with the Bluetooth passkey as required
        byte[] decryptedResponse = new byte[16];
        new Random().nextBytes(decryptedResponse);
        ByteBuffer pairingPasskeyBytes = ByteBuffer.allocate(4).order(ByteOrder.BIG_ENDIAN).putInt(
                mPairingPasskey);
        decryptedResponse[0] = 0x3;
        decryptedResponse[1] = pairingPasskeyBytes.get(1);
        decryptedResponse[2] = pairingPasskeyBytes.get(2);
        decryptedResponse[3] = pairingPasskeyBytes.get(3);

        mEncryptedResponse = encrypt(decryptedResponse);
        if (mEncryptedResponse == null) {
            return false;
        }
        mPasskeyCharacteristic.setValue(mEncryptedResponse);
        return true;

    }

    /**
     * Initialize all of the GATT characteristics with appropriate default values and the required
     * configurations.
     */
    void setup() {
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothDevice.ACTION_PAIRING_REQUEST);
        mContext.registerReceiver(mPairingAttemptsReceiver, filter);
        mModelIdCharacteristic = new BluetoothGattCharacteristic(FAST_PAIR_MODEL_ID_UUID.getUuid(),
                BluetoothGattCharacteristic.PROPERTY_READ,
                BluetoothGattCharacteristic.PERMISSION_READ);
        mModelIdCharacteristic.setValue(mModelId);
        mFastPairService.addCharacteristic(mModelIdCharacteristic);

        mKeyBasedPairingCharacteristic =
                new BluetoothGattCharacteristic(KEY_BASED_PAIRING_UUID.getUuid(),
                        BluetoothGattCharacteristic.PROPERTY_WRITE
                                | BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                        BluetoothGattCharacteristic.PERMISSION_WRITE);
        mKeyBasedPairingCharacteristic.setValue(mModelId);
        mKeyBasedPairingCharacteristic.addDescriptor(new BluetoothGattDescriptor(
                CLIENT_CHARACTERISTIC_CONFIG.getUuid(),
                BluetoothGattDescriptor.PERMISSION_READ
                        | BluetoothGattDescriptor.PERMISSION_WRITE));
        mFastPairService.addCharacteristic(mKeyBasedPairingCharacteristic);

        mPasskeyCharacteristic =
                new BluetoothGattCharacteristic(PASSKEY_UUID.getUuid(),
                        BluetoothGattCharacteristic.PROPERTY_WRITE
                                | BluetoothGattCharacteristic.PROPERTY_NOTIFY,
                        BluetoothGattCharacteristic.PERMISSION_WRITE);
        mPasskeyCharacteristic.setValue(mModelId);
        mPasskeyCharacteristic.addDescriptor(new BluetoothGattDescriptor(
                CLIENT_CHARACTERISTIC_CONFIG.getUuid(),
                BluetoothGattDescriptor.PERMISSION_READ
                        | BluetoothGattDescriptor.PERMISSION_WRITE));

        mFastPairService.addCharacteristic(mPasskeyCharacteristic);

        mAccountKeyCharacteristic =
                new BluetoothGattCharacteristic(ACCOUNT_KEY_UUID.getUuid(),
                        BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE,
                        BluetoothGattCharacteristic.PERMISSION_WRITE);
        mFastPairService.addCharacteristic(mAccountKeyCharacteristic);

        mDeviceNameCharacteristic =
                new BluetoothGattCharacteristic(DEVICE_NAME_CHARACTERISTIC_CONFIG.getUuid(),
                        BluetoothGattCharacteristic.PROPERTY_READ,
                        BluetoothGattCharacteristic.PERMISSION_READ);
        mDeviceNameCharacteristic.setValue(BluetoothAdapter.getDefaultAdapter().getName());
        mFastPairService.addCharacteristic(mDeviceNameCharacteristic);
    }

    void start() {
        if (mBluetoothGattServer == null) {
            return;
        }
        mBluetoothGattServer.addService(mFastPairService);
    }

    void stop() {
        if (mBluetoothGattServer == null) {
            return;
        }
        mBluetoothGattServer.removeService(mFastPairService);
    }

    void dump(IndentingPrintWriter writer) {
        writer.println("Currently connected to        : " + mRemoteGattDevice);
        writer.println("Successful pairing attempts   : " + mSuccessCount);
        writer.println("Unsuccessful pairing attempts : " + mFailureCount);
    }
}
