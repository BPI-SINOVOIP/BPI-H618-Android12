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

import com.android.tradefed.config.Option;
import com.android.tradefed.config.OptionClass;

import com.google.common.annotations.VisibleForTesting;

import java.util.HashSet;
import java.util.Set;

/** A package name provider that accepts package names via a command line option. */
@OptionClass(alias = "command-line-package-name-provider")
public final class CommandLinePackageNameProvider implements PackageNameProvider {
    @VisibleForTesting static final String PACKAGE = "package";

    @Option(name = PACKAGE, description = "App package names.")
    private final Set<String> mPackages = new HashSet<>();

    @Override
    public Set<String> get() {
        return mPackages;
    }
}
