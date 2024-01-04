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

import android.app.Fragment;
import android.os.Bundle;
import android.text.Editable;
import android.view.KeyEvent;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.EditText;
import android.text.TextUtils;
import android.text.InputFilter;
import android.widget.TextView;
import android.text.TextWatcher;
import android.text.method.PasswordTransformationMethod;
import android.util.Log;

import com.android.tv.launcher.R;

public class DeviceNameEditTextFragment extends Fragment
        implements TextWatcher, TextView.OnEditorActionListener {

	private static final String TAG = "DeviceNameEditTextFragment";
    private static final String EXTRA_LAYOUT_RES_ID = "layout_res_id";
    private static final String EXTRA_EDIT_TEXT_RES_ID = "edit_text_res_id";
    private static final String EXTRA_DESC = "description";
    private static final String EXTRA_INITIAL_TEXT = "initialText";
    private static final String EXTRA_PASSWORD = "password";
    private TextWatcher mTextWatcher = null;
    private TextView.OnEditorActionListener mEditorActionListener = null;

    private static final int BLUETOOTH_NAME_MAX_LENGTH_BYTES = 248;

    public static DeviceNameEditTextFragment newInstance(int layoutResId, int editTextResId) {
        DeviceNameEditTextFragment fragment = new DeviceNameEditTextFragment();
        Bundle args = new Bundle();
        if (layoutResId == 0 || editTextResId == 0) {
            throw new IllegalArgumentException("resource id must be valid values");
        }
        args.putInt(EXTRA_LAYOUT_RES_ID, layoutResId);
        args.putInt(EXTRA_EDIT_TEXT_RES_ID, editTextResId);
        fragment.setArguments(args);
        return fragment;
    }

    public static DeviceNameEditTextFragment newInstance(String description) {
        return newInstance(description, null);
    }

    public static DeviceNameEditTextFragment newInstance(String description, String initialText) {
        return newInstance(description, initialText, false);
    }

    public static DeviceNameEditTextFragment newInstance(String description, String initialText,
            boolean password) {
        DeviceNameEditTextFragment fragment = new DeviceNameEditTextFragment();
        Bundle args = new Bundle();
        args.putString(EXTRA_DESC, description);
        args.putString(EXTRA_INITIAL_TEXT, initialText);
        args.putBoolean(EXTRA_PASSWORD, password);
        fragment.setArguments(args);
        return fragment;

    }

    public void setTextWatcher(TextWatcher textWatcher) {
        mTextWatcher = textWatcher;
    }

    public void setOnEditorActionListener(TextView.OnEditorActionListener listener) {
        mEditorActionListener = listener;
    }

    @Override
    public View onCreateView(LayoutInflater inflater, ViewGroup container,
            Bundle savedInstanceState) {
        View view = null;
        EditText editText = null;
        int layoutResId = getArguments().getInt(EXTRA_LAYOUT_RES_ID, R.layout.bt_name_edittext_fragment);
        int editTextResId = getArguments().getInt(EXTRA_EDIT_TEXT_RES_ID, R.id.btname_edittext);

        view = inflater.inflate(layoutResId, container, false);
        editText = (EditText) view.findViewById(editTextResId);
        
        String descString = getArguments().getString(EXTRA_DESC);
        Log.d(TAG, "onCreateView descString="+descString);
        if (!TextUtils.isEmpty(descString)) {
            TextView description = (TextView) view.findViewById(R.id.btname_description);
            if (description != null) {
                description.setText(descString);
                description.setVisibility(View.VISIBLE);
            }
        }

        if (editText != null) {
            editText.setOnEditorActionListener(this);
            editText.addTextChangedListener(this);
            editText.setFilters(new InputFilter[] {
                new Utf8ByteLengthFilter(BLUETOOTH_NAME_MAX_LENGTH_BYTES)
            });
            editText.requestFocus();
            String initialText = getArguments().getString(EXTRA_INITIAL_TEXT);
            Log.d(TAG, "onCreateView initialText="+initialText);
            if(!TextUtils.isEmpty(initialText)) {
                editText.setText(initialText);
                editText.setSelection(initialText.length());
            }
            if (getArguments().getBoolean(EXTRA_PASSWORD, false)) {
                editText.setTransformationMethod(PasswordTransformationMethod.getInstance());
            }
        }
        return view;
    }

    @Override
    public void afterTextChanged(Editable s) {
        if (mTextWatcher != null) {
            mTextWatcher.afterTextChanged(s);
        }
    }

    @Override
    public void beforeTextChanged(CharSequence s, int start, int count, int after) {
        if (mTextWatcher != null) {
            mTextWatcher.beforeTextChanged(s, start, count, after);
        }
    }

    @Override
    public void onTextChanged(CharSequence s, int start, int before, int count) {
        if (mTextWatcher != null) {
            mTextWatcher.onTextChanged(s, start, before, count);
        }
    }

    @Override
    public boolean onEditorAction(TextView v, int actionId, KeyEvent event) {
        Log.v(TAG, "onEditorAction()...");
        if (mEditorActionListener != null) {
            return mEditorActionListener.onEditorAction(v, actionId, event);
        } else {
            return false;
        }
    }
}
