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

import com.google.common.annotations.VisibleForTesting;

import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.util.HashSet;
import java.util.Set;
import java.util.stream.Collectors;

/** A package name provider that accepts files that contains package names. */
public final class FileBasedPackageNameProvider implements PackageNameProvider {
    @VisibleForTesting static final String PACKAGES_FILE = "packages-file";
    @VisibleForTesting static final String COMMENT_LINE_PREFIX = "#";

    @Option(
            name = PACKAGES_FILE,
            description =
                    "File paths that contain package names separated by newline characters."
                        + " Comment lines are supported only if the lines start with double slash."
                        + " Trailing comments are not supported. Empty lines are ignored.")
    private final Set<File> mPackagesFiles = new HashSet<>();

    @Override
    public Set<String> get() throws IOException {
        Set<String> packages = new HashSet<>();
        for (File packagesFile : mPackagesFiles) {
            packages.addAll(
                    Files.readAllLines(packagesFile.toPath()).parallelStream()
                            .map(String::trim)
                            .filter(this::isPackageName)
                            .collect(Collectors.toSet()));
        }
        return packages;
    }

    private boolean isPackageName(String text) {
        // Check the text is not an empty string and not a comment line.
        return !text.isEmpty() && !text.startsWith(COMMENT_LINE_PREFIX);
    }
}
