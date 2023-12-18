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
package com.android.car.ui.sharedlibrary;

import android.content.Context;

import com.android.car.ui.sharedlibrary.oemapis.SharedLibraryVersionProviderOEMV1;

import com.chassis.car.ui.sharedlibrary.SharedLibraryFactoryImpl;

/**
 * An implementation of {@link SharedLibraryVersionProviderOEMV1} for the reference design
 * shared library.
 */
public class SharedLibraryVersionProviderImpl implements SharedLibraryVersionProviderOEMV1 {

    @Override
    public Object getSharedLibraryFactory(int maxVersion, Context context, String packageName) {
        return new SharedLibraryFactoryImpl(context);
    }
}
