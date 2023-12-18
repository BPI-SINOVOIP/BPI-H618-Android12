/*
 * Copyright (C) 2021 The Android Open Source Project
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

package com.android.tv.settings.service;

import android.app.Service;
import android.content.Intent;
import android.os.Bundle;
import android.os.Handler;
import android.os.IBinder;
import android.util.ArrayMap;
import android.util.Log;
import android.util.Pair;

import androidx.annotation.Nullable;

import com.android.tv.settings.service.ISettingsService;
import com.android.tv.settings.service.ISettingsServiceListener;
import com.android.tv.settings.service.data.State;
import com.android.tv.settings.service.data.StateUtil;
import com.android.tv.twopanelsettings.slices.ContextSingleton;

import java.util.List;

public class SettingsService extends Service {
    private static final String TAG = "TvSettingsService";
    private static final boolean DEBUG = true;
    private final Handler mHandler = new Handler();
    private final ArrayMap<Integer, Pair<State, Integer>> mStateMap = new ArrayMap<>();

    private ISettingsServiceListener mListener;

    private final ISettingsService.Stub mBinder = new ISettingsService.Stub() {

        @Override
        public List<PreferenceParcelable> getPreferences(int state) {
            return null;
        }

        @Override
        public PreferenceParcelable getPreference(int state, String key) {
            return null;
        }

        @Override
        public void registerListener(ISettingsServiceListener listener) {
            mHandler.post(() -> {
                mListener = listener;
            });
        }

        @Override
        public void unRegisterListener(ISettingsServiceListener listener) {
            mHandler.post(() -> {
                mListener = null;
            });
        }

        @Override
        public void onCreate(int state, Bundle extras) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onCreate " + state);
                }
                SettingsService.this.onCreateFragment(state, extras);
            });
        }

        @Override
        public void onStart(int state) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onStart " + state);
                }
                SettingsService.this.onStartFragment(state);
            });
        }

        @Override
        public void onResume(int state) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onResume " + state);
                }
                SettingsService.this.onResumeFragment(state);
            });

        }

        @Override
        public void onPause(int state) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onPause " + state);
                }
                SettingsService.this.onPauseFragment(state);
            });
        }

        @Override
        public void onStop(int state) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onStop " + state);
                }
                SettingsService.this.onStopFragment(state);
            });
        }

        @Override
        public void onDestroy(int state) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onDestroy " + state);
                }
                SettingsService.this.onDestroyFragment(state);
            });
        }

        @Override
        public void onPreferenceClick(int state, String key, boolean status) {
            mHandler.post(() -> {
                if (DEBUG) {
                    Log.d(TAG, "onPreferenceTreeClick " + state + " " + key);
                }
                SettingsService.this.onPreferenceClick(state, key, status);
            });
        }

        @Override
        public void grantSliceAccess(String packageName, String uri) {
            ContextSingleton.getInstance()
                    .grantFullAccess(getApplicationContext(), uri, packageName);
        }
    };

    void onCreateFragment(int state, Bundle extras) {
        StateUtil.createState(
                getApplicationContext(), state, mListener, mStateMap).onCreate(extras);
    }

    void onStartFragment(int state) {
        StateUtil.getState(state, mStateMap).onStart();
    }


    void onResumeFragment(int state) {
        StateUtil.getState(state, mStateMap).onResume();
    }

    void onStopFragment(int state) {
        StateUtil.getState(state, mStateMap).onStop();
    }

    void onPauseFragment(int state) {
        StateUtil.getState(state, mStateMap).onPause();
    }

    void onDestroyFragment(int state) {
        StateUtil.getState(state, mStateMap).onDestroy();
        StateUtil.removeState(state, mStateMap);
    }

    void onPreferenceClick(int state, String key, boolean status) {
        StateUtil.getState(state, mStateMap).onPreferenceTreeClick(key, status);
    }


    @Nullable
    @Override
    public IBinder onBind(Intent intent) {
        return mBinder;
    }
}
