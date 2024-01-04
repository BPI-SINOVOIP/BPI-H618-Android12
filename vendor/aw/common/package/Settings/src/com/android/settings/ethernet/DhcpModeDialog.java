/*
 * Copyright (C) 2011 The Android Open Source Project
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

package com.android.settings.ethernet;

import com.android.settings.R;

import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.os.Bundle;
import android.security.Credentials;
import android.security.KeyStore;
import android.text.Editable;
import android.text.TextWatcher;
import android.view.View;
import android.view.WindowManager;
import android.widget.AdapterView;
import android.widget.ArrayAdapter;
import android.widget.Button;
import android.widget.CheckBox;
import android.widget.Spinner;
import android.widget.TextView;
import android.widget.EditText;
import android.net.EthernetManager;
import android.net.InterfaceConfiguration;
import android.net.IpConfiguration;
import android.os.INetworkManagementService;
import android.util.Log;
import android.widget.CompoundButton;
import android.widget.CompoundButton.OnCheckedChangeListener;
import android.content.ContentResolver;
import android.provider.Settings;

import java.net.InetAddress;

public class DhcpModeDialog extends AlertDialog implements TextWatcher,
        View.OnClickListener, AdapterView.OnItemSelectedListener {

    private static final String TAG = "DhcpDialog";
    private final KeyStore mKeyStore = KeyStore.getInstance();
    private final DialogInterface.OnClickListener mListener;

    private boolean mSecondClick;

    private View mView;

    public DhcpModeDialog(Context context, DialogInterface.OnClickListener listener,
            IpConfiguration config, boolean secondClick) {
        super(context);
        mListener = listener;
        mSecondClick = secondClick;
    }

    @Override
    protected void onCreate(Bundle savedState) {
        mView = getLayoutInflater().inflate(R.layout.eth_dhcp_dialog, null);
        setTitle(R.string.eth_dhcp_configure);
        setView(mView);
        setInverseBackgroundForced(true);
        Context context = getContext();

        setButton(DialogInterface.BUTTON_NEGATIVE, context.getString(R.string.eth_cancel), mListener);
        setButton(DialogInterface.BUTTON_POSITIVE, context.getString(R.string.eth_ok), mListener);

        super.onCreate(savedState);
        mView.findViewById(R.id.eth_conf_editor).setVisibility(View.VISIBLE);
        getWindow().setSoftInputMode(WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE |
                WindowManager.LayoutParams.SOFT_INPUT_STATE_HIDDEN);

        if(mSecondClick) {
            getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(false);
        } else {
            getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(true);
        }
    }

    @Override
    public void afterTextChanged(Editable field) {
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        getButton(DialogInterface.BUTTON_POSITIVE).setEnabled(true);
    }

    @Override
    public void onItemSelected(AdapterView<?> parent, View view, int position, long id) {
    }

    @Override
    public void onNothingSelected(AdapterView<?> parent) {
    }

    @Override
    public void onClick(View v) {
    }

    public IpConfiguration getIpConfiguration() {
        return null;
    }

}
