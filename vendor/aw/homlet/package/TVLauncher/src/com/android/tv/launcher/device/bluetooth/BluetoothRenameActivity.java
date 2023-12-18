/*
 * Copyright (C) 2014 The Android Open Source Project
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

package com.android.tv.launcher.device.bluetooth;

import android.view.WindowManager;
import com.android.tv.launcher.R;
import com.android.tv.launcher.dialog.old.Action;
import com.android.tv.launcher.dialog.old.ActionAdapter;
import com.android.tv.launcher.dialog.old.ActionFragment;
import com.android.tv.launcher.dialog.old.DialogActivity;
import com.android.tv.launcher.dialog.old.EditTextFragment;

import android.app.Fragment;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.drawable.ColorDrawable;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.text.Html;
import android.text.InputFilter;
import android.text.InputType;
import android.text.InputFilter.LengthFilter;
import android.util.Log;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.widget.RelativeLayout;
import android.widget.TextView;
import android.view.inputmethod.EditorInfo;

import com.android.tv.launcher.util.AccessibilityHelper;

import android.bluetooth.BluetoothAdapter;

import java.util.ArrayList;
import java.util.Locale;

/**
 * BluetoothPairingDialog asks the user to enter a PIN / Passkey / simple
 * confirmation for pairing with a remote Bluetooth device.
 */
public class BluetoothRenameActivity extends DialogActivity {
    private static final String TAG = "BluetoothRenameActivity";

    private DeviceNameEditTextFragment mEditTextFragment;
    private Fragment mContentFragment;

    private RelativeLayout mTopLayout;
    protected ColorDrawable mBgDrawable = new ColorDrawable();
    private TextView mTitleText;
    private TextView mInstructionText;

    private BluetoothAdapter mBluetoothAdapter;

    public static final String ACTION_RENAME_REQUEST = "tv.launcher.action.bluetooth.rename.request";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        if (!ACTION_RENAME_REQUEST.equals(intent.getAction())) {
            Log.e(TAG, "Error: this activity may be started only with intent " + ACTION_RENAME_REQUEST);
            finish();
            return;
        }

        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
        createRenameDialog();

        ViewGroup contentView = (ViewGroup) findViewById(android.R.id.content);
        mTopLayout = (RelativeLayout) contentView.getChildAt(0);

        // Fade out the old activity, and fade in the new activity.
        overridePendingTransition(R.anim.fade_in, R.anim.fade_out);

        // Set the activity background
        int bgColor = getResources().getColor(R.color.dialog_activity_background);
        mBgDrawable.setColor(bgColor);
        mBgDrawable.setAlpha(255);
        mTopLayout.setBackground(mBgDrawable);

        // Make sure pairing wakes up day dream
        getWindow().addFlags(WindowManager.LayoutParams.FLAG_DISMISS_KEYGUARD |
                WindowManager.LayoutParams.FLAG_SHOW_WHEN_LOCKED |
                WindowManager.LayoutParams.FLAG_TURN_SCREEN_ON |
                WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
    }

    @Override
    protected void onResume() {
        super.onResume();


    }

    @Override
    protected void onPause() {
        dismiss();

        super.onPause();
    }


    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            dismiss();
        }
        return super.onKeyDown(keyCode, event);
    }


    private void dismiss() {
        finish();
    }

    private void setDeviceName(String deviceName) {
        Log.d(TAG, "Setting device name to " + deviceName);
        mBluetoothAdapter.setName(deviceName);
    }

    private void attachListeners(DeviceNameEditTextFragment editTextFragment) {

        editTextFragment.setOnEditorActionListener(new TextView.OnEditorActionListener() {

            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                Log.v(TAG, "onEditorAction().");

                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    setDeviceName(v.getText().toString());
                    dismiss();
                    return true;    // action handled
                } else {
                    return false;   // not handled
                }
            }
        });
    }



    private void createRenameDialog() {
        // Build a Dialog activity view, with EditText Fragment

        mEditTextFragment = DeviceNameEditTextFragment.newInstance(null, mBluetoothAdapter.getName());
        attachListeners(mEditTextFragment);

        mContentFragment = new Fragment() {
            @Override
            public View onCreateView(LayoutInflater inflater, ViewGroup container,
                    Bundle savedInstanceState) {
                View v = inflater.inflate(R.layout.bt_pairing_passkey_display, container, false);

                mTitleText = (TextView) v.findViewById(R.id.title);
                mInstructionText = (TextView) v.findViewById(R.id.pairing_instructions);

                mTitleText.setText(getString(R.string.accessory_rename) + "?");

                if (AccessibilityHelper.forceFocusableViews(getActivity())) {
                    mTitleText.setFocusable(true);
                    mTitleText.setFocusableInTouchMode(true);
                    mInstructionText.setFocusable(true);
                    mInstructionText.setFocusableInTouchMode(true);
                }

                String instructions = getString(R.string.accessory_bluetooth_rename_tip);
                
                mInstructionText.setText(Html.fromHtml(instructions));

                return v;
            }
        };

        setContentAndActionFragments(mContentFragment, mEditTextFragment);
    }



}
