/*
 * Copyright (C) 2017 The Android Open Source Project
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

import android.content.Context;
import androidx.preference.Preference;
import android.app.AlertDialog;
import android.util.Log;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;
import android.text.TextUtils;
import android.text.Editable;
import android.text.InputFilter;
import android.text.TextWatcher;
import android.text.BidiFormatter;
import android.view.View;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.WindowManager;
import android.view.inputmethod.EditorInfo;
import android.content.DialogInterface;
import android.bluetooth.BluetoothAdapter;

import com.android.tv.settings.R;
import com.android.settingslib.core.lifecycle.Lifecycle;
import com.android.settingslib.core.AbstractPreferenceController;

public class BluetoothDeviceRenamePreferenceController extends
        AbstractPreferenceController implements TextWatcher {

    public static final String PREF_KEY = "bt_rename_device";
    private static final String TAG = "BluetoothNamePrefCtrl";
    private static final int BLUETOOTH_NAME_MAX_LENGTH_BYTES = 248;
    EditText mDeviceNameView;
    private AlertDialog mAlertDialog;
    private Button mOkButton;
    private final Context mContext;
    BluetoothAdapter mBluetoothAdapter;

    // This flag is set when the name is updated by code, to distinguish from user changes
    private boolean mDeviceNameUpdated;
    // This flag is set when the user edits the name (preserved on rotation)
    private boolean mDeviceNameEdited;

    public BluetoothDeviceRenamePreferenceController(Context context) {
        super(context);
        mContext = context;
        mBluetoothAdapter = BluetoothAdapter.getDefaultAdapter();
    }

    @Override
    public String getPreferenceKey() {
        return PREF_KEY;
    }

    @Override
    public boolean isAvailable() {
        return mBluetoothAdapter != null;
    }

    @Override
    public void updateState(Preference preference) {
        preference.setSummary(getDeviceName());
        preference.setSelectable(true);
    }

    protected int getDialogTitle() {
        return R.string.bluetooth_rename_device;
    }

    protected String getDeviceName() {
        if (mBluetoothAdapter != null && mBluetoothAdapter.isEnabled()) {
            return mBluetoothAdapter.getName();
        }
        return null;
    }

    protected void setDeviceName(String deviceName) {
        mBluetoothAdapter.setName(deviceName);
    }

    public void afterTextChanged(Editable s) {
        mOkButton = mAlertDialog.getButton(DialogInterface.BUTTON_POSITIVE);
        mDeviceNameEdited = false;
        if (mDeviceNameUpdated) {
            // Device name changed by code; disable Ok button until edited by user
            mDeviceNameUpdated = false;
            mOkButton.setEnabled(false);
        } else {
            mDeviceNameEdited = true;
            if (mOkButton != null) {
                mOkButton.setEnabled(s.toString().trim().length() != 0);
            }
        }
    }

    void updateDeviceName() {
        String name = getDeviceName();
        if (name != null) {
            mDeviceNameUpdated = true;
            mDeviceNameEdited = false;
            mDeviceNameView.setText(name);
        }
    }

    private View createDialogView(String deviceName) {
        final LayoutInflater layoutInflater = (LayoutInflater)mContext
                    .getSystemService(Context.LAYOUT_INFLATER_SERVICE);
        View view = layoutInflater.inflate(R.layout.dialog_edittext, null);
        mDeviceNameView = (EditText) view.findViewById(R.id.edittext);
        mDeviceNameView.setFilters(new InputFilter[] {
                new Utf8ByteLengthFilter(BLUETOOTH_NAME_MAX_LENGTH_BYTES)
        });
        mDeviceNameView.setText(deviceName);    // set initial value before adding listener
        if (!TextUtils.isEmpty(deviceName)) {
            mDeviceNameView.setSelection(deviceName.length());
        }
        mDeviceNameView.addTextChangedListener(this);
        mDeviceNameView.setOnEditorActionListener(new TextView.OnEditorActionListener() {
            @Override
            public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
                if (actionId == EditorInfo.IME_ACTION_DONE) {
                    setDeviceName(v.getText().toString());
                    mAlertDialog.dismiss();
                    return true;    // action handled
                } else {
                    return false;   // not handled
                }
            }
        });
        return view;
    }

    @Override
    public boolean handlePreferenceTreeClick(Preference preference) {
        if (PREF_KEY.equals(preference.getKey())) {
            AlertDialog.Builder builder = new AlertDialog.Builder(mContext);
            builder.setTitle(getDialogTitle());
            builder.setView(createDialogView(getDeviceName()));
            builder.setPositiveButton(R.string.bluetooth_rename_button, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    setDeviceName(mDeviceNameView.getText().toString());
                    preference.setSummary(mDeviceNameView.getText().toString());
                    dialog.dismiss();
                }
            });

            builder.setNegativeButton(android.R.string.cancel, new DialogInterface.OnClickListener() {
                @Override
                public void onClick(DialogInterface dialog, int which) {
                    dialog.dismiss();
                }
            });
            mAlertDialog = builder.create();
            mAlertDialog.show();
            mAlertDialog.getWindow().setSoftInputMode(
                    WindowManager.LayoutParams.SOFT_INPUT_STATE_ALWAYS_VISIBLE);
            return true;
        }
        return false;
    }

    /* Not used */
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    /* Not used */
    public void onTextChanged(CharSequence s, int start, int before, int count) {
    }
}
