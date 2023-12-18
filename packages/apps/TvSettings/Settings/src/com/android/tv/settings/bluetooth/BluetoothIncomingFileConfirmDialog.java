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
package com.android.tv.settings.bluetooth;

import android.view.WindowManager;

import com.android.tv.settings.R;
import com.android.tv.settings.dialog.old.Action;
import com.android.tv.settings.dialog.old.ActionFragment;
import com.android.tv.settings.dialog.old.DialogActivity;
import com.android.tv.settings.util.AccessibilityHelper;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

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
import android.os.CountDownTimer;
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

import com.android.tv.settings.util.AccessibilityHelper;

import java.util.ArrayList;
import java.util.Locale;
import com.android.internal.logging.nano.MetricsProto;

public class BluetoothIncomingFileConfirmDialog extends DialogActivity {

    private static final String KEY_TRANSFER = "action_transfer";
    private static final String KEY_CANCEL = "action_cancel";

    static final String ACTION_ACCEPT = "android.btopp.intent.action.ACCEPT";
    private static final String TAG = "BluetoothIncomingFileConfirmDialog";
    private static final boolean DEBUG = false;

    private BluetoothDevice mDevice;

    private ActionFragment mActionFragment;
    private ArrayList<Action> mActions;

    private RelativeLayout mTopLayout;
    protected ColorDrawable mBgDrawable = new ColorDrawable();
    private long TRANSFER_TIMEOUT = 50000;
    CountDownTimer mCountDownTimer = new CountDownTimer(TRANSFER_TIMEOUT, 1000) {
        @Override
        public void onTick(long l) {
        }

        @Override
        public void onFinish() {
            finish();
        }
    };

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        Intent intent = getIntent();
        if (!BluetoothDevice.ACTION_INCOMINGFILE_CONFIRM_REQUEST.equals(intent.getAction())) {
            Log.e(TAG, "Error: this activity may be started only with intent " +
                    BluetoothDevice.ACTION_INCOMINGFILE_CONFIRM_REQUEST);
            finish();
            return;
        }

        mDevice = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
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

    @Deprecated
    public int getMetricsCategory() {
        return MetricsProto.MetricsEvent.BLUETOOTH_DIALOG_FRAGMENT;
    }

    @Override
    protected void onResume() {
        super.onResume();
        mCountDownTimer.start();
    }

    @Override
    protected void onPause() {
        mCountDownTimer.cancel();
        dismiss();
        super.onPause();
    }

    @Override
    public void onActionClicked(Action action) {
        String key = action.getKey();
        if (KEY_TRANSFER.equals(key)) {
            onTransfer();
            dismiss();
        } else if (KEY_CANCEL.equals(key)) {
            CancelingTransfer();
        }
    }

    @Override
    public boolean onKeyDown(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK) {
            CancelingTransfer();
        }
        return super.onKeyDown(keyCode, event);
    }

    private ArrayList<Action> getActions() {
        ArrayList<Action> actions = new ArrayList<Action>();
        actions.add(new Action.Builder()
                .key(KEY_TRANSFER)
                .title(getString(R.string.bluetooth_incoming_confirm))
                .build());

        actions.add(new Action.Builder()
                .key(KEY_CANCEL)
                .title(getString(R.string.bluetooth_cancel))
                .build());
        return actions;
    }

    private void cancelPairing() {
        if (DEBUG) {
            Log.d(TAG, "cancelPairing");
        }
    }


    private void createConfirmationDialog() {
        mActions = getActions();

        mActionFragment = ActionFragment.newInstance(mActions);
        final Fragment contentFragment =
                ConfirmationDialogFragment.newInstance(mDevice);
        setContentAndActionFragments(contentFragment, mActionFragment);
    }


    private void onTransfer(){
        Intent intent = new Intent().setAction(BluetoothDevice.ACTION_INCOMINGFILE_CONFIRM_ACCEPT);
        sendBroadcast(intent);
    }

    private void CancelingTransfer(){
        dismiss();
    }

    private void dismiss() {
        finish();
    }

    public static class ConfirmationDialogFragment extends Fragment {

        private static final String ARG_DEVICE = "ConfirmationDialogFragment.TRANSFER_DEVICE";

        private BluetoothDevice mDevice;

        public static ConfirmationDialogFragment newInstance(BluetoothDevice device) {
            final ConfirmationDialogFragment fragment = new ConfirmationDialogFragment();
            final Bundle b = new Bundle(1);
            b.putParcelable(ARG_DEVICE, device);
            fragment.setArguments(b);
            return fragment;
        }

        @Override
        public void onCreate(@Nullable Bundle savedInstanceState) {
            super.onCreate(savedInstanceState);

            final Bundle args = getArguments();
            mDevice = args.getParcelable(ARG_DEVICE);
        }

        @Override
        public View onCreateView(LayoutInflater inflater, ViewGroup container,
                Bundle savedInstanceState) {
            final View v = inflater.inflate(R.layout.bt_incomingfile_confirm_display, container, false);

            final TextView mTitleText = (TextView) v.findViewById(R.id.title);
            final TextView mInstructionText = (TextView) v.findViewById(R.id.transfer_instructions);

            mTitleText.setText(getString(R.string.bluetooth_incomingfile_request));

            if (AccessibilityHelper.forceFocusableViews(getActivity())) {
                mTitleText.setFocusable(true);
                mTitleText.setFocusableInTouchMode(true);
                mInstructionText.setFocusable(true);
                mInstructionText.setFocusableInTouchMode(true);
            }

            String mDeviceName = mDevice != null ? mDevice.getName():"default";
            final String instructions = getString(R.string.bluetooth_incoming_confirm_msg,
                                mDeviceName);
            mInstructionText.setText(Html.fromHtml(instructions));
            return v;
        }
    }

}
