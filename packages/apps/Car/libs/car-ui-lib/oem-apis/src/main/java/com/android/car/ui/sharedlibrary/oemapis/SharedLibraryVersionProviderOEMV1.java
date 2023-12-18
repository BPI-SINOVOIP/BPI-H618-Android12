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
package com.android.car.ui.sharedlibrary.oemapis;

import android.content.Context;

/**
 * An interface for objects that support providing a list a supported versions of
 * {@link SharedLibraryFactoryOEMV1} to the app. See {@link #getSharedLibraryFactory(int)}}
 * for more information.
 */
public interface SharedLibraryVersionProviderOEMV1 {
    /**
     * Returns an object that implements {@link SharedLibraryFactoryOEMV1} or a later version.
     *
     * OEMs should aim to return the highest version of the factory possible that is <=
     * {@code maxVersion}. If the shared library is not able to provide that version,
     * it may return null, in which case car-ui-lib will fall back to it's static,
     * uncustomized implementation.
     *
     * The shared library may also choose to return different SharedLibraryFactories based on
     * certain conditions, like what type of device this is, or what app it's being used in.
     * (The app can be discovered via {@link android.app.Application#getProcessName()}
     *
     * @param maxVersion The maximum version of {@link SharedLibraryFactoryOEMV1} supported by the
     *                   app.
     * @param context The shared library's context. It uses the shared library's classloader,
     *                so layout inflaters created from it can use views defined in the shared lib.
     * @param packageName The package name of the app creating the shared library. Can be used
     *                    to provide per-app customizations.
     *
     * @return An object implementing {@link SharedLibraryFactoryOEMV1} for a version <=
     *         {@code maxVersion}.
     */
    Object getSharedLibraryFactory(int maxVersion, Context context, String packageName);
}
