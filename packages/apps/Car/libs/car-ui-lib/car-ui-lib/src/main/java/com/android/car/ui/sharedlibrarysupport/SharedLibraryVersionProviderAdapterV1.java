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
package com.android.car.ui.sharedlibrarysupport;

import android.content.Context;

import com.android.car.ui.sharedlibrary.oemapis.SharedLibraryVersionProviderOEMV1;

/**
 * This class is an wrapper around {@link SharedLibraryVersionProviderOEMV1} that implements
 * {@link SharedLibraryVersionProvider}, to provide a version-agnostic way of interfacing with
 * the OEM's SharedLibraryFactoryVersionProvider.
 */
final class SharedLibraryVersionProviderAdapterV1 implements SharedLibraryVersionProvider {

    private SharedLibraryVersionProviderOEMV1 mOemProvider;

    SharedLibraryVersionProviderAdapterV1(
            SharedLibraryVersionProviderOEMV1 oemVersionProvider) {
        mOemProvider = oemVersionProvider;
    }

    @Override
    public Object getSharedLibraryFactory(int maxVersion, Context context, String packageName) {
        return mOemProvider.getSharedLibraryFactory(maxVersion, context, packageName);
    }
}
