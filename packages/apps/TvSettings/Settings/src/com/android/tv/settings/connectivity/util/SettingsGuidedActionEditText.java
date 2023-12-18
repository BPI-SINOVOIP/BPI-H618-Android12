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
import android.util.Log;
import android.view.KeyEvent;
import android.view.accessibility.AccessibilityNodeInfo;
import android.widget.EditText;
import android.widget.TextView;
import android.view.View;
import android.view.MotionEvent;
import android.view.inputmethod.InputMethodManager;
import androidx.leanback.widget.ImeKeyMonitor;

/**
 * Mostly copied from {@GuidedActionEditText}, remove some code to satisfies TvSettings need.
 */
public class SettingsGuidedActionEditText extends EditText implements ImeKeyMonitor {

    private static final String TAG = "SettingsGuidedActionEditText";
    private ImeKeyMonitor.ImeKeyListener mKeyListener;
    private View mLeaderView;

    public SettingsGuidedActionEditText(Context context) {
        super(context);
    }

    public SettingsGuidedActionEditText(Context context, AttributeSet attrs) {
        super(context, attrs);
    }

    public SettingsGuidedActionEditText(Context context, AttributeSet attrs, int defStyle) {
        super(context, attrs, defStyle);
    }

    @Override
    public void setImeKeyListener(ImeKeyMonitor.ImeKeyListener listener) {
        mKeyListener = listener;
    }

    @Override
    public boolean onKeyPreIme(int keyCode, KeyEvent event) {
        boolean result = false;
        if (mKeyListener != null) {
            result = mKeyListener.onKeyPreIme(this, keyCode, event);
        }
        if (!result) {
            result = super.onKeyPreIme(keyCode, event);
        }
        if (keyCode == KeyEvent.KEYCODE_BACK
            && event.getAction() == KeyEvent.ACTION_DOWN) {
            setFocusable(false);
        }
        return result;
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
    public void onInitializeAccessibilityNodeInfo(AccessibilityNodeInfo info) {
        super.onInitializeAccessibilityNodeInfo(info);
        info.setClassName(isFocused() ? EditText.class.getName() : TextView.class.getName());
    }

    @Override
    protected void onFocusChanged(boolean focused, int direction, Rect previouslyFocusedRect) {
        super.onFocusChanged(focused, direction, previouslyFocusedRect);
        // Make the TextView focusable during editing, avoid the TextView gets accessibility focus
        // before editing started. see also GuidedActionAdapterGroup where setFocusable(true).
        if (!focused) {
            //setFocusable(false);
            setFocus(false);
        }
    }
}
