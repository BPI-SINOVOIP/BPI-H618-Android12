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

package com.android.cts.verifier.companion;

import android.bluetooth.BluetoothDevice;
import android.companion.AssociationRequest;
import android.companion.BluetoothDeviceFilter;
import android.companion.CompanionDeviceManager;
import android.content.Intent;
import android.content.IntentSender;
import android.content.pm.PackageManager;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.widget.Button;
import android.widget.Toast;

import com.android.compatibility.common.util.CddTest;
import com.android.cts.verifier.PassFailButtons;
import com.android.cts.verifier.R;

import java.util.List;

/**
 * Test that Companion Device Awake {@link CompanionDeviceService} API is functional
 */
@CddTest(requirement = "3.16/C-1-2,C-1-3,H-1-1")
public class CompanionDeviceServiceTestActivity extends PassFailButtons.Activity{
    private static final String LOG_TAG = "=CDMSerivceTestActivity";
    private static final int REQUEST_CODE_CHOOSER = 0;

    private CompanionDeviceManager mCompanionDeviceManager;

    private Handler mHandler;

    private Button mPresentButton;
    private Button mGoneButton;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setContentView(R.layout.companion_service_test_main);
        setPassFailButtonClickListeners();
        mPresentButton = (Button) findViewById(R.id.present_button);
        mGoneButton = (Button) findViewById(R.id.gone_button);
        mPresentButton.setEnabled(false);
        mGoneButton.setEnabled(false);

        mCompanionDeviceManager = getSystemService(CompanionDeviceManager.class);

        mHandler = new Handler();

        getPassButton().setEnabled(false);

        findViewById(R.id.go_button).setOnClickListener(v -> associateDevices());

    }

    private void associateDevices() {
        if (!getApplicationContext().getPackageManager().hasSystemFeature(
                PackageManager.FEATURE_BLUETOOTH)) {
            Log.d(LOG_TAG, "PackageManager.FEATURE_BLUETOOTH not supported."
                    + "This test case is not applicable");
            getPassButton().setEnabled(true);
            return;
        }

        AssociationRequest request = new AssociationRequest.Builder()
                .addDeviceFilter(new BluetoothDeviceFilter.Builder().build())
                .build();

        CompanionDeviceManager.Callback callback = new CompanionDeviceManager.Callback() {
            @Override
            public void onDeviceFound(IntentSender chooserLauncher) {
                try {
                    startIntentSenderForResult(chooserLauncher,
                            REQUEST_CODE_CHOOSER, null, 0, 0, 0);
                } catch (IntentSender.SendIntentException e) {
                    fail(e);
                }
            }

            @Override
            public void onFailure(CharSequence error) {
                fail(error);
            }
        };
        mCompanionDeviceManager.associate(request, callback, null);
    }

    private void fail(Throwable reason) {
        Log.e(LOG_TAG, "Test failed", reason);
        fail(reason.getMessage());
    }

    private void fail(CharSequence reason) {
        Toast.makeText(this, reason, Toast.LENGTH_LONG).show();
        Log.e(LOG_TAG, reason.toString());
        setTestResultAndFinish(false);
    }

    private void disassociate(String deviceAddress) {
        mCompanionDeviceManager.stopObservingDevicePresence(deviceAddress);
        mCompanionDeviceManager.disassociate(deviceAddress);
        List<String> associations = mCompanionDeviceManager.getAssociations();

        if (associations.contains(deviceAddress)) {
            fail("Disassociating device " + deviceAddress
                    + " did not remove it from associations list"
                    + associations);
            return;
        }
    }

    private void isGoneTest(String deviceAddress) {
        if (Boolean.FALSE.equals(DevicePresenceListener.sDeviceNearBy)) {
            findViewById(R.id.present_button).setOnClickListener(
                    v -> isPresentTest(deviceAddress));
            mHandler.postDelayed(() -> mPresentButton.setEnabled(true), 5000);
        } else {
            disassociate(deviceAddress);
            fail("Device " + deviceAddress + " should be gone");
        }
    }

    private void isPresentTest(String deviceAddress) {
        if (Boolean.TRUE.equals(DevicePresenceListener.sDeviceNearBy)) {
            getPassButton().setEnabled(true);
            disassociate(deviceAddress);
        } else {
            disassociate(deviceAddress);
            fail("Device " + deviceAddress + " should be present");
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, Intent data) {
        if (requestCode == REQUEST_CODE_CHOOSER) {
            if (resultCode != RESULT_OK) {
                fail("Activity result code " + resultCode);
                return;
            }

            BluetoothDevice associatedDevice = data.getParcelableExtra(
                    CompanionDeviceManager.EXTRA_DEVICE);
            String deviceAddress = associatedDevice.getAddress();
            if (deviceAddress != null) {
                findViewById(R.id.gone_button).setOnClickListener(
                        v -> isGoneTest(deviceAddress));
                mCompanionDeviceManager.startObservingDevicePresence(deviceAddress);
                mHandler.postDelayed(() -> mGoneButton.setEnabled(true), 10000);
            } else {
                fail("The device was present but its address was null");
            }

        } else super.onActivityResult(requestCode, resultCode, data);
    }
}
