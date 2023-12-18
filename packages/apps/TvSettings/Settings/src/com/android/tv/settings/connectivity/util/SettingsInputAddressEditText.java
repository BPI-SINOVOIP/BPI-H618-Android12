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

package com.android.tv.settings.connectivity.util;

import android.content.Context;
import android.graphics.Rect;
import android.text.InputType;
import android.util.AttributeSet;
import android.view.KeyEvent;
import android.widget.EditText;
import android.util.Log;
import android.view.View;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;

public class SettingsInputAddressEditText extends EditText {

    private static final String TAG = "SettingsInputAddressEditText";
    private View mLeaderView;

    public SettingsInputAddressEditText(Context context) {
        super(context);
    }

    public SettingsInputAddressEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public SettingsInputAddressEditText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    public void setLeaderView(View leaderView) {
        this.mLeaderView = leaderView;
        if (mLeaderView != null) {
            mLeaderView.setFocusable(false);
            mLeaderView.setOnKeyListener(new View.OnKeyListener() {
                @Override
                public boolean onKey(View v, int keyCode, KeyEvent event) {
                    if (keyCode == KeyEvent.KEYCODE_DPAD_CENTER) {
                        Log.d(TAG,"click ok btn.");
                        setFocus(true);
                    }
                    return false;
                }
            });
            mLeaderView.setOnGenericMotionListener(new View.OnGenericMotionListener() {
                @Override
                public boolean onGenericMotion(View view, MotionEvent event) {
                    if(event.getActionButton() == MotionEvent.BUTTON_PRIMARY &&
                            event.getActionMasked() == MotionEvent.ACTION_BUTTON_PRESS) {
                        Log.d(TAG,"touch left mouse.");
                        setFocus(true);
                    }
                    return true;
                }
            });
        }
    }

    public void moveCursorToLast() {
        String hint = getText().toString();
        if (hint != null && hint.length() != 0) {
            setSelection(hint.length());
        }
    }

   private void setFocus(boolean focus) {
        this.setFocusable(focus);
        this.setFocusableInTouchMode(focus);
        if (mLeaderView != null) {
            mLeaderView.setFocusable(!focus);
            mLeaderView.setFocusableInTouchMode(!focus);
        }
        if (focus) {
            InputMethodManager inputManager = (InputMethodManager) getContext().getSystemService(Context.INPUT_METHOD_SERVICE);
            inputManager.showSoftInput(this, 0);
            moveCursorToLast();
            this.requestFocus();
        }
        this.setInputType(focus ? InputType.TYPE_CLASS_TEXT : InputType.TYPE_NULL);
    }


    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        if (keyCode == KeyEvent.KEYCODE_BACK
            && event.getAction() == KeyEvent.ACTION_DOWN) {
            setFocus(false);
        }
        return false;
    }

}
