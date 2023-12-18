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

import com.android.tv.settings.service.ISettingsServiceListener;
import com.android.tv.settings.service.PreferenceParcelable;
import android.os.Bundle;

interface ISettingsService {
  List<PreferenceParcelable> getPreferences(int state);
  PreferenceParcelable getPreference(int state, String key);
  void registerListener(ISettingsServiceListener listener);
  void unRegisterListener(ISettingsServiceListener listener);
  void onCreate(int state, in Bundle extras);
  void onStart(int state);
  void onResume(int state);
  void onPause(int state);
  void onStop(int state);
  void onDestroy(int state);
  void onPreferenceClick(int state, String key, boolean status);
  void grantSliceAccess(String packageName, String uri);
}