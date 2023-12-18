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

package com.android.tv.settings.service.data;

import android.util.ArrayMap;

import com.android.tv.settings.service.PreferenceParcelable;

import java.util.List;
import java.util.Map;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/** Manage preference parcelables. */
public class PreferenceParcelableManager {
    private final Map<String, PreferenceParcelable> prefParcelables;

    public PreferenceParcelableManager() {
        prefParcelables = new ArrayMap<>();
    }

    public void addPrefParcelable(PreferenceParcelable preferenceParcelable) {
        prefParcelables.put(getKey(preferenceParcelable.getKey()), preferenceParcelable);
    }

    public PreferenceParcelable getOrCreatePrefParcelable(String key) {
        return getOrCreatePrefParcelable(new String[]{key});
    }

    public PreferenceParcelable getOrCreatePrefParcelable(String[] key) {
        String compoundKey = getKey(key);
        if (!prefParcelables.containsKey(compoundKey)) {
            prefParcelables.put(compoundKey, new PreferenceParcelable(key));
        }
        return prefParcelables.get(compoundKey);
    }

    public PreferenceParcelable prefParcelableCopy(String[] key) {
        return prefParcelables.get(getKey(key)).immutableCopy();
    }

    public PreferenceParcelable prefParcelableCopy(String key) {
        PreferenceParcelable preferenceParcelable = prefParcelables.get(getKey(new String[]{key}));
        if (preferenceParcelable != null) {
            return preferenceParcelable.immutableCopy();
        }
        return null;
    }

    public static PreferenceParcelable prefParcelableCopy(
            PreferenceParcelable preferenceParcelable) {
        return preferenceParcelable.immutableCopy();
    }

    public static List<PreferenceParcelable> prefParcelablesCopy(List<PreferenceParcelable> prefs) {
        return prefs.stream().map(pref -> pref.immutableCopy()).collect(Collectors.toList());
    }

    private static String getKey(String[] key) {
        return Stream.of(key).collect(Collectors.joining(" "));
    }
}
