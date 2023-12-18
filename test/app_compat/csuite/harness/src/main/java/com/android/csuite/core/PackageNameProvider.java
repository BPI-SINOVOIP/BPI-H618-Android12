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

package com.android.csuite.core;

import java.io.IOException;
import java.util.Set;

/** Provides a list of package names. */
public interface PackageNameProvider {
    /**
     * Returns a set of package names.
     *
     * @return the package names. An empty set is returned if no package names are to be provided.
     * @throws IOException if failed to get package names.
     */
    Set<String> get() throws IOException;
}
