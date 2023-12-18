/*
 * Copyright (C) 2018 The Android Open Source Project
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

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.res.Resources;
import android.os.Handler;
import android.util.IndentingPrintWriter;
import android.util.Slog;

import com.android.car.CarLog;
import com.android.car.CarServiceUtils;
import com.android.car.R;

/**
 * An advertiser for the Bluetooth LE based Fast Pair service. FastPairProvider enables easy
 * Bluetooth pairing between a peripheral and a phone participating in the Fast Pair Seeker role.
 * When the seeker finds a compatible peripheral a notification prompts the user to begin pairing if
 * desired.  A peripheral should call startAdvertising when it is appropriate to pair, and
 * stopAdvertising when pairing is complete or it is no longer appropriate to pair.
 */
public class FastPairProvider {
    private static final String TAG = CarLog.tagFor(FastPairProvider.class);
    private static final boolean DBG = FastPairUtils.DBG;

    private final int mModelId;
    private final String mAntiSpoofKey;
    private final boolean mAutomaticAcceptance;
    private final Context mContext;
    private boolean mStarted;
    private int mScanMode;
    private FastPairAdvertiser mFastPairModelAdvertiser;
    private FastPairAdvertiser mFastPairAccountAdvertiser;
    private FastPairGattServer mFastPairGattServer;
    private Handler mFastPairAdvertiserHandler;

    FastPairAdvertiser.Callbacks mAdvertiserCallbacks = new FastPairAdvertiser.Callbacks() {
        @Override
        public void onRpaUpdated(BluetoothDevice device) {
            mFastPairGattServer.updateLocalRpa(device);
        }
    };

    FastPairGattServer.Callbacks mGattServerCallbacks = new FastPairGattServer.Callbacks() {
        @Override
        public void onPairingCompleted(boolean successful) {
            if (DBG) {
                Slog.d(TAG, "onPairingCompleted " + successful);
            }
            if (successful || mScanMode != BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
                advertiseAccountKeys();
            }
        }

    };

    /**
     * listen for changes in the Bluetooth adapter specifically for the Bluetooth adapter turning
     * on, turning off, and changes to discoverability
     */
    BroadcastReceiver mDiscoveryModeChanged = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            String action = intent.getAction();
            if (DBG) {
                Slog.d(TAG, "onReceive, " + action);
            }
            switch (action) {
                case BluetoothAdapter.ACTION_SCAN_MODE_CHANGED:
                    mScanMode = intent
                            .getIntExtra(BluetoothAdapter.EXTRA_SCAN_MODE,
                                    BluetoothAdapter.SCAN_MODE_NONE);
                    if (DBG) {
                        Slog.d(TAG, "NewScanMode = " + mScanMode);
                    }
                    if (mScanMode == BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE) {
                        advertiseModelId();
                    } else if (mScanMode == BluetoothAdapter.SCAN_MODE_CONNECTABLE
                            && mFastPairGattServer != null
                            && !mFastPairGattServer.isConnected()) {
                        // The adapter is no longer discoverable, and the Fast Pair session is
                        // complete
                        advertiseAccountKeys();
                    }
                    break;

                case BluetoothAdapter.ACTION_STATE_CHANGED:
                    int state = intent
                            .getIntExtra(BluetoothAdapter.EXTRA_STATE, BluetoothAdapter.STATE_OFF);
                    if (state != BluetoothAdapter.STATE_ON) {
                        if (mFastPairGattServer != null) {
                            mFastPairGattServer.stop();
                            mFastPairGattServer = null;
                        }
                    }
                    break;
            }
        }
    };

    /**
     * FastPairProvider constructor which loads Fast Pair variables from the device specific
     * resource overlay.
     *
     * @param context user specific context on which all Bluetooth operations shall occur.
     */
    public FastPairProvider(Context context) {
        mContext = context;
        Resources res = mContext.getResources();

        mModelId = res.getInteger(R.integer.fastPairModelId);
        mAntiSpoofKey = res.getString(R.string.fastPairAntiSpoofKey);
        mAutomaticAcceptance = res.getBoolean(R.bool.fastPairAutomaticAcceptance);
    }

    /**
     * Start the Fast Pair provider which will register for Bluetooth broadcasts.
     */
    public void start() {
        if (mModelId == 0) {
            Slog.w(TAG, "Model ID undefined, disabling");
            return;
        }
        Slog.d(TAG, "modelId == " + mModelId);
        mFastPairAdvertiserHandler = new Handler(
                CarServiceUtils.getHandlerThread(FastPairUtils.THREAD_NAME).getLooper());
        IntentFilter filter = new IntentFilter();
        filter.addAction(BluetoothAdapter.ACTION_SCAN_MODE_CHANGED);
        filter.addAction(BluetoothAdapter.ACTION_STATE_CHANGED);
        mContext.registerReceiver(mDiscoveryModeChanged, filter);

        mStarted = true;
    }

    /**
     * Stop the Fast Pair provider which will unregister the broadcast receiver.
     */
    public void stop() {
        if (mStarted) {
            mContext.unregisterReceiver(mDiscoveryModeChanged);
            mStarted = false;
        }
    }

    void advertiseModelId() {
        mFastPairAdvertiserHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mFastPairAccountAdvertiser != null) {
                    mFastPairAccountAdvertiser.stopAdvertising();
                }
                if (mFastPairModelAdvertiser == null) {
                    mFastPairModelAdvertiser = new FastPairAdvertiser(mContext, mModelId,
                            mAdvertiserCallbacks);
                }

                startGatt();
                mFastPairModelAdvertiser.advertiseModelId();
            }
        });
    }

    void advertiseAccountKeys() {
        mFastPairAdvertiserHandler.post(new Runnable() {
            @Override
            public void run() {
                if (mFastPairModelAdvertiser != null) {
                    mFastPairModelAdvertiser.stopAdvertising();
                }
                if (mFastPairAccountAdvertiser == null) {
                    mFastPairAccountAdvertiser = new FastPairAdvertiser(mContext, mModelId,
                            mAdvertiserCallbacks);
                }

                startGatt();
                mFastPairAccountAdvertiser.advertiseAccountKeys();
            }
        });

    }

    void startGatt() {
        if (mFastPairGattServer == null) {
            mFastPairGattServer = new FastPairGattServer(mContext, mModelId,
                    mAntiSpoofKey,
                    mGattServerCallbacks,
                    mAutomaticAcceptance);
            mFastPairGattServer.start();
        }
    }

    /**
     * Dump current status of the Fast Pair provider
     *
     * @param writer
     */
    public void dump(IndentingPrintWriter writer) {
        writer.println(TAG + " services");
        if (mModelId == 0) {
            writer.increaseIndent();
            writer.println("Service Disabled");
            writer.decreaseIndent();
            return;
        }
        writer.increaseIndent();
        writer.println("Model ID                      : " + mModelId);
        if (mFastPairModelAdvertiser != null) {
            mFastPairModelAdvertiser.dump(writer);
        }
        if (mFastPairAccountAdvertiser != null) {
            mFastPairAccountAdvertiser.dump(writer);
        }
        if (mFastPairGattServer != null) {
            mFastPairGattServer.dump(writer);
        }
        writer.decreaseIndent();
    }
}
