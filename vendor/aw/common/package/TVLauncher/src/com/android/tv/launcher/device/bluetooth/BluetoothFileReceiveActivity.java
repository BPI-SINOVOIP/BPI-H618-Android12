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

import com.android.tv.launcher.util.AccessibilityHelper;

import java.util.ArrayList;
import java.util.Locale;

/**
 * BluetoothPairingDialog asks the user to enter a PIN / Passkey / simple
 * confirmation for pairing with a remote Bluetooth device.
 */
public class BluetoothFileReceiveActivity extends DialogActivity {

    private static final String KEY_YES = "action_yes";
    private static final String KEY_NO = "action_no";

    private static final String TAG = "BluetoothFileReceiveActivity";


    private ActionFragment mActionFragment;
    private Fragment mContentFragment;
    private ArrayList<Action> mActions;

    private RelativeLayout mTopLayout;
    protected ColorDrawable mBgDrawable = new ColorDrawable();
    private TextView mTitleText;
    private TextView mInstructionText;
    private TextView mFileInfoText;
    private EditText mTextInput;

    public static final String ACTION_FILE_RECEIVE_CONFIRM = "tv.launcher.action.bluetooth.file.receive.confirm";


    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        Intent intent = getIntent();
        if (!ACTION_FILE_RECEIVE_CONFIRM.equals(intent.getAction())) {
            Log.e(TAG, "Error: this activity may be started only with intent " + ACTION_FILE_RECEIVE_CONFIRM);
            finish();
            return;
        }

        




        mActions = new ArrayList<Action>();

        createConfirmationDialog();

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
    public void onActionClicked(Action action) {
        String key = action.getKey();
        if (KEY_YES.equals(key)) {
            dismiss();
        } else if (KEY_NO.equals(key)) {
            dismiss();
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            dismiss();
        }
        return super.onKeyDown(keyCode, event);
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<Action>();

        actions.add(new Action.Builder()
                        .key(KEY_YES)
                        .title(getString(R.string.accessory_bluetooth_unsave_yes))
                        .build());
        actions.add(new Action.Builder()
                        .key(KEY_NO)
                        .title(getString(R.string.accessory_bluetooth_unsave_no))
                        .build());
        return actions;
    }


    private void dismiss() {
        finish();
    }



    private void createConfirmationDialog() {
        // Build a Dialog activity view, with Action Fragment

        mActions = getActions();

        mActionFragment = ActionFragment.newInstance(mActions);
        mContentFragment = new Fragment() {
            @Override
            public View onCreateView(LayoutInflater inflater, ViewGroup container,
                    Bundle savedInstanceState) {
                View v = inflater.inflate(R.layout.bt_file_confirm_display, container, false);

                mTitleText = (TextView) v.findViewById(R.id.bt_file_devicename);
                mInstructionText = (TextView) v.findViewById(R.id.bt_file_tip);
                mFileInfoText = (TextView) v.findViewById(R.id.bt_file_info);

                if (AccessibilityHelper.forceFocusableViews(getActivity())) {
                    mTitleText.setFocusable(true);
                    mTitleText.setFocusableInTouchMode(true);
                    mInstructionText.setFocusable(true);
                    mInstructionText.setFocusableInTouchMode(true);
                    mFileInfoText.setFocusable(true);
                    mFileInfoText.setFocusableInTouchMode(true);
                }

                //String instructions = getString(R.string.accessory_bluetooth_unsave_tip);
                
                mTitleText.setText("device name");

                mInstructionText.setText("tip");

                mFileInfoText.setText("file info");

                return v;
            }
        };

        setContentAndActionFragments(mContentFragment, mActionFragment);
    }



}
