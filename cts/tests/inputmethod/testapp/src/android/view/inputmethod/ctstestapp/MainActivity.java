/*
 * Copyright (C) 2020 The Android Open Source Project
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
package android.view.inputmethod.ctstestapp;

import static android.view.WindowManager.LayoutParams.FLAG_ALT_FOCUSABLE_IM;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_PAN;
import static android.view.WindowManager.LayoutParams.SOFT_INPUT_ADJUST_RESIZE;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.net.Uri;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.view.Gravity;
import android.view.inputmethod.InputMethodManager;
import android.widget.EditText;
import android.widget.LinearLayout;
import android.widget.TextView;

import androidx.annotation.Nullable;

/**
 * A test {@link Activity} that automatically shows the input method.
 */
public final class MainActivity extends Activity {

    private static final String EXTRA_KEY_PRIVATE_IME_OPTIONS =
            "android.view.inputmethod.ctstestapp.EXTRA_KEY_PRIVATE_IME_OPTIONS";
    private static final String EXTRA_KEY_SHOW_DIALOG =
            "android.view.inputmethod.ctstestapp.EXTRA_KEY_SHOW_DIALOG";

    private static final String EXTRA_DISMISS_DIALOG = "extra_dismiss_dialog";
    private static final String EXTRA_SHOW_SOFT_INPUT = "extra_show_soft_input";

    private static final String ACTION_TRIGGER = "broadcast_action_trigger";
    private AlertDialog mDialog;
    private EditText mEditor;
    private final Handler mHandler = new Handler(Looper.myLooper());

    private BroadcastReceiver mBroadcastReceiver;

    @Nullable
    private String getStringIntentExtra(String key) {
        if (getPackageManager().isInstantApp()) {
            final Uri uri = getIntent().getData();
            if (uri == null || !uri.isHierarchical()) {
                return null;
            }
            return uri.getQueryParameter(key);
        }
        return getIntent().getStringExtra(key);
    }

    private boolean getBooleanIntentExtra(String key) {
        if (getPackageManager().isInstantApp()) {
            final Uri uri = getIntent().getData();
            if (uri == null || !uri.isHierarchical()) {
                return false;
            }
            return uri.getBooleanQueryParameter(key, false);
        }
        return getIntent().getBooleanExtra(key, false);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        final LinearLayout layout = new LinearLayout(this);
        layout.setOrientation(LinearLayout.VERTICAL);
        final boolean needShowDialog = getBooleanIntentExtra(EXTRA_KEY_SHOW_DIALOG);

        if (needShowDialog) {
            layout.setOrientation(LinearLayout.VERTICAL);
            layout.setGravity(Gravity.BOTTOM);
            getWindow().setSoftInputMode(SOFT_INPUT_ADJUST_RESIZE);

            final TextView textView = new TextView(this);
            textView.setText("This is DialogActivity");
            layout.addView(textView);

            mDialog = new AlertDialog.Builder(this)
                    .setView(new LinearLayout(this))
                    .create();
            mDialog.getWindow().addFlags(FLAG_ALT_FOCUSABLE_IM);
            mDialog.getWindow().setSoftInputMode(SOFT_INPUT_ADJUST_PAN);
            mDialog.show();
        } else {
            mEditor = new EditText(this);
            mEditor.setHint("editText");
            final String privateImeOptions = getStringIntentExtra(EXTRA_KEY_PRIVATE_IME_OPTIONS);
            if (privateImeOptions != null) {
                mEditor.setPrivateImeOptions(privateImeOptions);
            }
            mEditor.requestFocus();
            layout.addView(mEditor);
        }

        setContentView(layout);
    }

    @Override
    protected void onStart() {
        super.onStart();
        mBroadcastReceiver = new BroadcastReceiver() {
            @Override
            public void onReceive(Context context, Intent intent) {
                final Bundle extras = intent.getExtras();
                if (extras == null) {
                    return;
                }

                if (extras.containsKey(EXTRA_SHOW_SOFT_INPUT)) {
                    getSystemService(InputMethodManager.class).showSoftInput(mEditor, 0);
                }

                if (extras.getBoolean(EXTRA_DISMISS_DIALOG, false)) {
                    if (mDialog != null) {
                        mDialog.dismiss();
                        mDialog = null;
                    }
                    mHandler.postDelayed(() -> finish(), 100);
                }
            }
        };
        registerReceiver(mBroadcastReceiver, new IntentFilter(ACTION_TRIGGER));
    }

    @Override
    protected void onStop() {
        super.onStop();
        if (mBroadcastReceiver != null) {
            unregisterReceiver(mBroadcastReceiver);
            mBroadcastReceiver = null;
        }
    }
}
