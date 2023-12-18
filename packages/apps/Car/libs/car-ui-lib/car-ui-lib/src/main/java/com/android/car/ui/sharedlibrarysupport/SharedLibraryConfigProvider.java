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
package com.android.car.ui.sharedlibrarysupport;

import java.util.Set;

/**
 * This interface can be implemented on an app's {@link android.app.Application} class
 * in order to provide additional options to consider when loading the shared library.
 */
public interface SharedLibraryConfigProvider {

    /**
     * Returns a set of {@link SharedLibrarySpecifier SharedLibrarySpecifiers}. If a shared library
     * is in this set, it will not be loaded. The fallback implementation in the static version of
     * car-ui-lib will be used instead.
     */
    Set<SharedLibrarySpecifier> getSharedLibraryDenyList();
}
