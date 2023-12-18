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

package com.android.bedstead.nene.utils;

import android.os.Build;

import java.lang.reflect.Field;

/** SDK Version checks. */
public final class Versions {

    /** Any version. */
    public static final int ANY = -1;

    private static final String DEVELOPMENT_CODENAME = "S";

    private Versions() {

    }

    /**
     * Throw a {@link UnsupportedOperationException} if the minimum version requirement is not met.
     */
    public static void requireMinimumVersion(int min) {
        if (!meetsSdkVersionRequirements(min, ANY)) {
            throw new UnsupportedOperationException(
                    "Thie feature is only available on "
                            + versionToLetter(min)
                            + "+ (currently " + Build.VERSION.CODENAME + ")");
        }
    }

    private static String versionToLetter(int version) {
        for (Field field : Build.VERSION_CODES.class.getFields()) {
            if (!java.lang.reflect.Modifier.isStatic(field.getModifiers())) {
                continue;
            }
            if (!field.getType().equals(int.class)) {
                continue;
            }
            try {
                int fieldValue = (int) field.get(null);

                if (fieldValue == version) {
                    return field.getName();
                }
            } catch (IllegalAccessException e) {
                // Couldn't access this variable - ignore
            }
        }

        throw new IllegalStateException("Could not find version with code " + version);
    }

    /**
     * {@code true} if the minimum version requirement is met.
     */
    public static boolean meetsMinimumSdkVersionRequirement(int min) {
        return meetsSdkVersionRequirements(min, ANY);
    }

    /**
     * {@code true} if the minimum and maximum version requirements are met.
     *
     * <p>Use {@link #ANY} to accept any version.
     */
    public static boolean meetsSdkVersionRequirements(int min, int max) {
        if (min != ANY) {
            if (min == Build.VERSION_CODES.CUR_DEVELOPMENT) {
                if (!Build.VERSION.CODENAME.equals(DEVELOPMENT_CODENAME)) {
                    return false;
                }
            } else if (min > Build.VERSION.SDK_INT) {
                return false;
            }
        }

        if (max != ANY
                && max != Build.VERSION_CODES.CUR_DEVELOPMENT
                && max < Build.VERSION.SDK_INT) {
            return false;
        }

        return true;
    }
}
