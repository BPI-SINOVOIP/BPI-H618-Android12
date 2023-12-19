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

package com.android.textclassifier.testing;

import android.provider.DeviceConfig.Properties;
import androidx.annotation.NonNull;
import com.android.textclassifier.common.TextClassifierSettings;
import java.util.HashMap;
import javax.annotation.Nullable;

/** A fake DeviceConfig implementation for testing purpose. */
public final class TestingDeviceConfig implements TextClassifierSettings.IDeviceConfig {

  private final HashMap<String, String> strConfigs;
  private final HashMap<String, Boolean> boolConfigs;

  public TestingDeviceConfig() {
    this.strConfigs = new HashMap<>();
    this.boolConfigs = new HashMap<>();
  }

  public void setConfig(String key, String value) {
    strConfigs.put(key, value);
  }

  public void setConfig(String key, boolean value) {
    boolConfigs.put(key, value);
  }

  @Override
  public Properties getProperties(@NonNull String namespace, @NonNull String... names) {
    Properties.Builder builder = new Properties.Builder(namespace);
    for (String key : strConfigs.keySet()) {
      builder.setString(key, strConfigs.get(key));
    }
    for (String key : boolConfigs.keySet()) {
      builder.setBoolean(key, boolConfigs.get(key));
    }
    return builder.build();
  }

  @Override
  public boolean getBoolean(@NonNull String namespace, @NonNull String name, boolean defaultValue) {
    return boolConfigs.containsKey(name) ? boolConfigs.get(name) : defaultValue;
  }

  @Override
  public String getString(
      @NonNull String namespace, @NonNull String name, @Nullable String defaultValue) {
    return strConfigs.containsKey(name) ? strConfigs.get(name) : defaultValue;
  }
}
