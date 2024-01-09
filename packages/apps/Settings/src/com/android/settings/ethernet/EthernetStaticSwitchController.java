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

package com.android.settings.ethernet;

import android.content.Context;
import android.net.EthernetManager;
import android.util.Log;

import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.OnLifecycleEvent;
import androidx.preference.PreferenceScreen;

import com.android.settings.widget.GenericSwitchController;
import com.android.settings.widget.SwitchWidgetController;
import com.android.settingslib.RestrictedSwitchPreference;
import com.android.settingslib.core.AbstractPreferenceController;
/**
 * This controller helps to manage the state of ethernet switch preference.
 */
public class EthernetStaticSwitchController extends AbstractPreferenceController implements
        LifecycleObserver, SwitchWidgetController.OnSwitchChangeListener {

    private static final String TAG = "EthernetStaticSwitchController";
    public static final String KEY = "main_toggle_ethernet_static";

    private RestrictedSwitchPreference mPreference;
    private SwitchWidgetController mSwitchWidgetController;
    private boolean mListeningToOnSwitchChange = false;
    private boolean mStateMachineEvent = false;
    private SwitchWidgetController mSwitchWidget;
    private Context mContext;

    public EthernetStaticSwitchController(Context context, Lifecycle lifecycle) {
        super(context);
        if (lifecycle == null) {
            throw new IllegalArgumentException("Lifecycle must be set");
        }
        mContext = context;
        lifecycle.addObserver(this);
    }

    @Override
    public String getPreferenceKey() {
        return KEY;
    }

    @Override
    public boolean isAvailable() {
        return true;
    }

    @Override
    public void displayPreference(PreferenceScreen screen) {
        super.displayPreference(screen);
        mPreference = screen.findPreference(getPreferenceKey());
    }

    /** Lifecycle.Event.ON_START */
    @OnLifecycleEvent(Lifecycle.Event.ON_START)
    public void onStart() {
        if (mPreference != null) {
            mSwitchWidgetController = new GenericSwitchController(mPreference);
            if (!mListeningToOnSwitchChange) {
                mSwitchWidgetController.setListener(this);
                mListeningToOnSwitchChange = true;
            }
            if (mSwitchWidget != null) mSwitchWidget.setupView();
        }
    }

    /** Lifecycle.Event.ON_STOP */
    @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
    public void onStop() {
        if (mListeningToOnSwitchChange) {
            mSwitchWidget.stopListening();
            mListeningToOnSwitchChange = false;
        }
        if (mSwitchWidget != null) mSwitchWidget.teardownView();
    }

    /** Lifecycle.Event.ON_RESUME */
    @OnLifecycleEvent(Lifecycle.Event.ON_RESUME)
    public void onResume() {
        if (!mListeningToOnSwitchChange) {
            mSwitchWidgetController.setListener(this);
            mListeningToOnSwitchChange = true;
        }
    }

    /** Lifecycle.Event.ON_PAUSE */
    @OnLifecycleEvent(Lifecycle.Event.ON_PAUSE)
    public void onPause() {
        if (mListeningToOnSwitchChange) {
            if (mSwitchWidget != null) mSwitchWidget.stopListening();
            mListeningToOnSwitchChange = false;
        }
    }

    private void setSwitchBarChecked(boolean checked) {
        mStateMachineEvent = true;
        if (mSwitchWidget != null) mSwitchWidget.setChecked(checked);
        mStateMachineEvent = false;
    }

    @Override
    public boolean onSwitchToggled(boolean isChecked) {
        Log.w(TAG, "onSwitchToggled mStateMachineEvent: " + mStateMachineEvent);
        //Do nothing if called as a result of a state machine event
        if (mStateMachineEvent) {
            return true;
        }
        if (mSwitchWidget != null) mSwitchWidget.setChecked(isChecked);
        return true;
    }
}
