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
package com.android.car.ui.paintbooth;

import android.app.Application;
import android.content.Context;

import com.android.car.ui.sharedlibrarysupport.SharedLibraryConfigProvider;
import com.android.car.ui.sharedlibrarysupport.SharedLibrarySpecifier;

import java.util.Collections;
import java.util.Set;
import java.util.stream.Collectors;

/**
 * A {@link Application} subclass that implements {@link SharedLibraryConfigProvider},
 * allowing PaintBooth to disable the shared library.
 */
@SuppressWarnings("AndroidJdkLibsChecker")
public class PaintBoothApplication extends Application implements SharedLibraryConfigProvider {
    public static final String SHARED_PREFERENCES_FILE = "paintbooth_shared_prefs";
    public static final String SHARED_PREFERENCES_SHARED_LIB_DENYLIST =
            "paintbooth_shared_lib_deny";

    @Override
    public Set<SharedLibrarySpecifier> getSharedLibraryDenyList() {
        return getSharedPreferences(SHARED_PREFERENCES_FILE, Context.MODE_PRIVATE)
                .getStringSet(SHARED_PREFERENCES_SHARED_LIB_DENYLIST, Collections.emptySet())
                .stream()
                .map(packageName -> SharedLibrarySpecifier.builder()
                        .setPackageName(packageName)
                        .build())
                .collect(Collectors.toSet());
    }
}
